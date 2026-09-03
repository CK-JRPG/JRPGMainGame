#include "Combat/Characters/PartyActorSpawnSubsystem.h"

#include "Combat/Characters/CharacterRuntimeStateAdapter.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

void UPartyActorSpawnSubsystem::Deinitialize()
{
	InvalidateSpawnRequest();

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	TArray<ACombatCharacterActor*> Remaining;
	for (const TPair<FName, TObjectPtr<ACombatCharacterActor>>& Pair : SpawnedActorMap)
	{
		if (IsValid(Pair.Value.Get()))
		{
			Remaining.Add(Pair.Value.Get());
		}
	}

	// World teardown is not a successful battle boundary. Preserve the last committed state.
	DiscardCombatActors(Remaining);
	SpawnedActorMap.Reset();
	Super::Deinitialize();
}

void UPartyActorSpawnSubsystem::InvalidateSpawnRequest()
{
	++SpawnRequestSerial;
	ActiveSpawnRequestId = 0;
	bSpawnRequestInFlight = false;
	PendingSpawnIds.Reset();

	if (SpawnLoadHandle.IsValid())
	{
		SpawnLoadHandle->CancelHandle();
		SpawnLoadHandle.Reset();
	}
}

bool UPartyActorSpawnSubsystem::HasOwnedCombatActors() const
{
	for (const TPair<FName, TObjectPtr<ACombatCharacterActor>>& Pair : SpawnedActorMap)
	{
		if (IsValid(Pair.Value.Get()))
		{
			return true;
		}
	}
	return false;
}

// ========= SpawnEntry 레지스트리 =========

void UPartyActorSpawnSubsystem::RegisterSpawnEntry(const FCharacterSpawnEntry& Entry)
{
	if (Entry.CharacterID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::RegisterSpawnEntry : 실패 - CharacterID가 None임."));
		return;
	}

	SpawnEntryMap.Add(Entry.CharacterID, Entry);
	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::RegisterSpawnEntry : 등록 - %s"), *Entry.CharacterID.ToString());
}

void UPartyActorSpawnSubsystem::RegisterSpawnEntries(const TArray<FCharacterSpawnEntry>& Entries)
{
	for (const FCharacterSpawnEntry& Entry : Entries)
	{
		RegisterSpawnEntry(Entry);
	}
}

// ========= 에셋 프리로드 ===========

void UPartyActorSpawnSubsystem::PreloadAssets(const TArray<FName>& PartyIds)
{
	TArray<FSoftObjectPath> Paths = CollectSoftPaths(PartyIds);
	if (Paths.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::PreloadAssets : 실패 - 로드할 에셋이 없음."));
		return;
	}

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	PreloadHandle = StreamableManager.RequestAsyncLoad(Paths, []()
	{
		UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 에셋 사전 로드 완료."));
	}, FStreamableManager::AsyncLoadHighPriority);
}

// ========== CombatCharacterActor 비동기 스폰 ============

void UPartyActorSpawnSubsystem::AsyncSpawnCombatActorsAtFieldPositions(
	const TArray<FName>& PartyIds,
	const TMap<FName, FTransform>& FieldTransforms,
	TFunction<bool(TArray<ACombatCharacterActor*>)> OnComplete)
{
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::AsyncSpawnCombatActorsAtFieldPositions : 실패 - PartyIds가 비어 있음."));
		if (OnComplete)
		{
			OnComplete({});
		}
		return;
	}

	if (bSpawnRequestInFlight || HasOwnedCombatActors())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PartyActorSpawnSubsystem::AsyncSpawnCombatActorsAtFieldPositions : 이미 스폰 요청 또는 소유 Actor가 있어 중복 요청을 거부합니다."));
		if (OnComplete)
		{
			OnComplete({});
		}
		return;
	}

	TSet<FName> UniquePartyIds;
	TMap<FName, FCharacterSpawnEntry> RequestedEntries;
	for (const FName& ID : PartyIds)
	{
		if (ID.IsNone() || UniquePartyIds.Contains(ID))
		{
			UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 유효하지 않거나 중복된 Party ID - %s"), *ID.ToString());
			if (OnComplete)
			{
				OnComplete({});
			}
			return;
		}

		const FCharacterSpawnEntry* Entry = SpawnEntryMap.Find(ID);
		if (!Entry || Entry->ActorClass.IsNull())
		{
			UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : SpawnEntry 또는 ActorClass 없음 - %s"), *ID.ToString());
			if (OnComplete)
			{
				OnComplete({});
			}
			return;
		}

		UniquePartyIds.Add(ID);
		RequestedEntries.Add(ID, *Entry);
	}

	if (SpawnLoadHandle.IsValid())
	{
		SpawnLoadHandle->ReleaseHandle();
		SpawnLoadHandle.Reset();
	}

	const uint64 RequestId = ++SpawnRequestSerial;
	ActiveSpawnRequestId = RequestId;
	bSpawnRequestInFlight = true;
	PendingSpawnIds = UniquePartyIds;

	auto AreRequestedActorClassesLoaded = [RequestedEntries]() -> bool
	{
		for (const TPair<FName, FCharacterSpawnEntry>& Pair : RequestedEntries)
		{
			if (!Pair.Value.ActorClass.Get())
			{
				return false;
			}
		}
		return true;
	};

	TWeakObjectPtr<UPartyActorSpawnSubsystem> WeakThis(this);
	auto DoSpawn = [WeakThis, RequestId, PartyIds, FieldTransforms, RequestedEntries, OnComplete]()
	{
		UPartyActorSpawnSubsystem* Self = WeakThis.Get();
		if (!Self || !Self->bSpawnRequestInFlight || Self->ActiveSpawnRequestId != RequestId)
		{
			return;
		}

		UWorld* World = Self->GetWorld();
		if (!World)
		{
			Self->InvalidateSpawnRequest();
			if (OnComplete)
			{
				OnComplete({});
			}
			return;
		}

		UCharacterRuntimeSubsystem* CharacterRuntime = nullptr;
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			CharacterRuntime = GameInstance->GetSubsystem<UCharacterRuntimeSubsystem>();
		}

		TArray<ACombatCharacterActor*> SpawnedActors;
		SpawnedActors.Reserve(PartyIds.Num());
		const FTransform* LeaderTransform = FieldTransforms.Find(PartyIds[0]);
		auto DestroyUnownedActor = [World](ACombatCharacterActor* Actor)
		{
			if (!IsValid(Actor))
			{
				return;
			}

			World->GetTimerManager().ClearAllTimersForObject(Actor);
			if (AController* Controller = Actor->GetController())
			{
				Controller->UnPossess();
				if (!Controller->IsPlayerController())
				{
					Controller->Destroy();
				}
			}
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
			Actor->SetActorTickEnabled(false);
			Actor->Destroy();
		};

		auto MakeFallbackTransformNearLeader = [LeaderTransform](int32 PartyIndex) -> FTransform
		{
			if (!LeaderTransform)
			{
				return FTransform::Identity;
			}

			const int32 CompanionIndex = FMath::Max(1, PartyIndex);
			float AngleOffset = (CompanionIndex % 2 != 0) ? -45.0f : 45.0f;
			AngleOffset *= FMath::CeilToFloat(CompanionIndex / 2.0f);

			const FVector LeaderLocation = LeaderTransform->GetLocation();
			const FVector Direction = LeaderTransform->GetUnitAxis(EAxis::X).RotateAngleAxis(AngleOffset, FVector::UpVector);
			return FTransform(LeaderTransform->GetRotation(), LeaderLocation - (Direction * 200.0f));
		};

		for (const FName& ID : PartyIds)
		{
			const FCharacterSpawnEntry* Entry = RequestedEntries.Find(ID);
			if (!Entry)
			{
				continue;
			}

			TSubclassOf<ACombatCharacterActor> LoadedClass = Entry->ActorClass.Get();
			if (!LoadedClass)
			{
				UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 클래스 로드 실패 - %s"), *ID.ToString());
				continue;
			}

			FTransform ActorTransform;
			if (const FTransform* FieldTransform = FieldTransforms.Find(ID))
			{
				ActorTransform = *FieldTransform;
			}
			else
			{
				const int32 PartyIndex = PartyIds.IndexOfByKey(ID);
				ActorTransform = MakeFallbackTransformNearLeader(PartyIndex);
				ActorTransform.AddToTranslation(Entry->SpawnOffset);
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 필드 트랜스폼 없음, 리더 주변 폴백 사용 - %s"), *ID.ToString());
			}

			ACombatCharacterActor* SpawnedActor = Self->SpawnSingleActor(LoadedClass, ActorTransform);
			if (!IsValid(SpawnedActor))
			{
				UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 스폰 실패 - %s"), *ID.ToString());
				continue;
			}

			if (SpawnedActor->GetCombatantId() != ID)
			{
				UE_LOG(LogTemp, Error,
					TEXT("PartyActorSpawnSubsystem : 요청 ID와 Actor CombatantId 불일치 - Requested=%s Actor=%s"),
					*ID.ToString(), *SpawnedActor->GetCombatantId().ToString());
				DestroyUnownedActor(SpawnedActor);
				continue;
			}

			if (CharacterRuntime)
			{
				if (const FCharacterRuntimeState* State = CharacterRuntime->GetState(ID))
				{
					if (!FCharacterRuntimeStateAdapter::Restore(SpawnedActor, *State))
					{
						UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 런타임 상태 복원 실패 - %s"), *ID.ToString());
						DestroyUnownedActor(SpawnedActor);
						continue;
					}
				}
			}

			Self->SpawnedActorMap.Add(ID, SpawnedActor);
			SpawnedActors.Add(SpawnedActor);
			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 필드 위치 기반 스폰 성공 - %s"), *ID.ToString());
		}

		const bool bAcceptedByEncounter = OnComplete && OnComplete(SpawnedActors);

		Self = WeakThis.Get();
		if (!Self || !Self->bSpawnRequestInFlight || Self->ActiveSpawnRequestId != RequestId)
		{
			return;
		}

		if (bAcceptedByEncounter)
		{
			for (const FName& ID : PartyIds)
			{
				Self->PendingSpawnIds.Remove(ID);
			}
		}
		else
		{
			Self->DiscardCombatActors(SpawnedActors);
		}

		Self->PendingSpawnIds.Reset();
		Self->bSpawnRequestInFlight = false;
		Self->ActiveSpawnRequestId = 0;
		if (Self->SpawnLoadHandle.IsValid())
		{
			Self->SpawnLoadHandle->ReleaseHandle();
			Self->SpawnLoadHandle.Reset();
		}
	};

	if (AreRequestedActorClassesLoaded())
	{
		DoSpawn();
		return;
	}

	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(RequestedEntries.Num());
	for (const TPair<FName, FCharacterSpawnEntry>& Pair : RequestedEntries)
	{
		Paths.AddUnique(Pair.Value.ActorClass.ToSoftObjectPath());
	}

	if (Paths.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 스폰용 로드 경로가 없습니다."));
		InvalidateSpawnRequest();
		if (OnComplete)
		{
			OnComplete({});
		}
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	SpawnLoadHandle = StreamableManager.RequestAsyncLoad(
		Paths,
		[DoSpawn]() { DoSpawn(); },
		FStreamableManager::AsyncLoadHighPriority);

	if (!SpawnLoadHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 비동기 로드 요청 생성 실패."));
		InvalidateSpawnRequest();
		if (OnComplete)
		{
			OnComplete({});
		}
	}
}

// ============= CombatCharacterActor 파괴 ===============

void UPartyActorSpawnSubsystem::DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors)
{
	DespawnCombatActorsInternal(Actors, true);
}

void UPartyActorSpawnSubsystem::DiscardCombatActors(const TArray<ACombatCharacterActor*>& Actors)
{
	DespawnCombatActorsInternal(Actors, false);
}

void UPartyActorSpawnSubsystem::DespawnCombatActorsInternal(
	const TArray<ACombatCharacterActor*>& Actors,
	bool bCommitRuntimeState)
{
	TSet<ACombatCharacterActor*> RequestedActors;
	for (ACombatCharacterActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			RequestedActors.Add(Actor);
		}
	}

	TArray<FName> OwnedIds;
	TArray<ACombatCharacterActor*> OwnedActors;
	TSet<ACombatCharacterActor*> MatchedActors;
	for (auto It = SpawnedActorMap.CreateIterator(); It; ++It)
	{
		ACombatCharacterActor* Actor = It.Value().Get();
		if (!IsValid(Actor))
		{
			PendingSpawnIds.Remove(It.Key());
			It.RemoveCurrent();
			continue;
		}

		if (RequestedActors.Contains(Actor))
		{
			OwnedIds.Add(It.Key());
			OwnedActors.Add(Actor);
			MatchedActors.Add(Actor);
		}
	}

	for (ACombatCharacterActor* RequestedActor : RequestedActors)
	{
		if (!MatchedActors.Contains(RequestedActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 소유하지 않은 Actor despawn 요청 무시 - %s"),
				*GetNameSafe(RequestedActor));
		}
	}

	UCharacterRuntimeSubsystem* CharacterRuntime = nullptr;
	UWorld* World = GetWorld();
	if (World)
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			CharacterRuntime = GameInstance->GetSubsystem<UCharacterRuntimeSubsystem>();
		}
	}

	// Phase 1: capture every owned, non-provisional actor before any lifecycle mutation.
	TMap<FName, FCharacterRuntimeState> CapturedStates;
	bool bCanDestroyCommittedActors = true;
	int32 CommittedActorCount = 0;
	if (bCommitRuntimeState)
	{
		if (!CharacterRuntime)
		{
			bCanDestroyCommittedActors = false;
			UE_LOG(LogTemp, Error,
				TEXT("PartyActorSpawnSubsystem : Runtime State Store가 없어 정상 despawn을 중단합니다."));
		}

		for (int32 Index = 0; Index < OwnedActors.Num(); ++Index)
		{
			ACombatCharacterActor* Actor = OwnedActors[Index];
			const FName CharacterID = OwnedIds[Index];
			if (PendingSpawnIds.Contains(CharacterID))
			{
				continue;
			}
			++CommittedActorCount;

			if (Actor->GetCombatantId() != CharacterID)
			{
				bCanDestroyCommittedActors = false;
				UE_LOG(LogTemp, Error,
					TEXT("PartyActorSpawnSubsystem : despawn ID 불일치로 정상 despawn 중단 - Owned=%s Actor=%s"),
					*CharacterID.ToString(), *Actor->GetCombatantId().ToString());
				continue;
			}

			FCharacterRuntimeState CapturedState;
			if (!FCharacterRuntimeStateAdapter::Capture(Actor, CapturedState))
			{
				bCanDestroyCommittedActors = false;
				UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 런타임 상태 capture 실패, 정상 despawn 중단 - %s"),
					*CharacterID.ToString());
				continue;
			}

			CapturedStates.Add(CharacterID, CapturedState);
		}

		if (CommittedActorCount > 0
			&& (CapturedStates.Num() != CommittedActorCount
				|| !CharacterRuntime
				|| !CharacterRuntime->CommitStates(CapturedStates)))
		{
			bCanDestroyCommittedActors = false;
			UE_LOG(LogTemp, Error,
				TEXT("PartyActorSpawnSubsystem : 전체 Runtime State batch commit 실패. 해당 Actor를 파괴하지 않습니다."));
		}
	}

	// Phase 2: only after capture/commit, release runtime ownership and destroy exact owned actors.
	int32 DespawnedCount = 0;
	for (int32 Index = 0; Index < OwnedActors.Num(); ++Index)
	{
		ACombatCharacterActor* Actor = OwnedActors[Index];
		const FName CharacterID = OwnedIds[Index];
		if (!IsValid(Actor))
		{
			continue;
		}
		const bool bIsProvisionalActor = PendingSpawnIds.Contains(CharacterID);
		if (bCommitRuntimeState && !bIsProvisionalActor && !bCanDestroyCommittedActors)
		{
			continue;
		}

		if (World)
		{
			World->GetTimerManager().ClearAllTimersForObject(Actor);
		}

		if (AController* Controller = Actor->GetController())
		{
			if (Controller->IsPlayerController())
			{
				UE_LOG(LogTemp, Warning, TEXT("DespawnCombatActors : PlayerController 빙의 해제 - %s"), *Actor->GetName());
				Controller->UnPossess();
			}
			else
			{
				Controller->UnPossess();
				Controller->Destroy();
			}
		}

		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		Actor->SetActorTickEnabled(false);

		if (const TObjectPtr<ACombatCharacterActor>* OwnedActor = SpawnedActorMap.Find(CharacterID))
		{
			if (OwnedActor->Get() == Actor)
			{
				SpawnedActorMap.Remove(CharacterID);
			}
		}
		PendingSpawnIds.Remove(CharacterID);

		Actor->Destroy();
		++DespawnedCount;
	}

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : %d개 소유 액터 디스폰 완료 (StateCommit=%s)."),
		DespawnedCount, bCommitRuntimeState ? TEXT("true") : TEXT("false"));
}

// ========== 액터 및 캐릭터 ID 조회 ============

TArray<ACombatCharacterActor*> UPartyActorSpawnSubsystem::GetSpawnedActors() const
{
	TArray<ACombatCharacterActor*> Result;
	for (const TPair<FName, TObjectPtr<ACombatCharacterActor>>& Pair : SpawnedActorMap)
	{
		if (IsValid(Pair.Value.Get()))
		{
			Result.Add(Pair.Value.Get());
		}
	}
	return Result;
}

ACombatCharacterActor* UPartyActorSpawnSubsystem::FindActorByCharacterID(const FName& CharacterID) const
{
	if (const TObjectPtr<ACombatCharacterActor>* Found = SpawnedActorMap.Find(CharacterID))
	{
		return Found->Get();
	}
	return nullptr;
}

// ====== 유틸 ========

ACombatCharacterActor* UPartyActorSpawnSubsystem::SpawnSingleActor(
	TSubclassOf<ACombatCharacterActor> ActorClass, const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	return World->SpawnActor<ACombatCharacterActor>(ActorClass, SpawnTransform, SpawnParams);
}

TArray<FSoftObjectPath> UPartyActorSpawnSubsystem::CollectSoftPaths(const TArray<FName>& PartyIds) const
{
	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(PartyIds.Num());

	for (const FName& ID : PartyIds)
	{
		if (const FCharacterSpawnEntry* Entry = SpawnEntryMap.Find(ID))
		{
			if (!Entry->ActorClass.IsNull())
			{
				Paths.AddUnique(Entry->ActorClass.ToSoftObjectPath());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::CollectSoftPaths : %s의 ActorClass가 null."), *ID.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::CollectSoftPaths : 매핑 없음 - %s"), *ID.ToString());
		}
	}

	return Paths;
}
