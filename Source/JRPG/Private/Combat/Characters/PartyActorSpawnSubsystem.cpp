#include "Combat/Characters/PartyActorSpawnSubsystem.h"

#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Engine/AssetManager.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UPartyActorSpawnSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	TArray<ACombatCharacterActor*> Remaining;
	for (auto& Pair : SpawnedActorMap)
		if (Pair.Value.Get()) Remaining.Add(Pair.Value.Get());
	DespawnCombatActors(Remaining);
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
		RegisterSpawnEntry(Entry);
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
		PreloadHandle->ReleaseHandle();
	
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
	TFunction<void(TArray<ACombatCharacterActor*>)> OnComplete)
{
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::AsyncSpawnCombatActorsAtFieldPositions : 실패 - PartyIds가 비어 있음."));
		if (OnComplete) OnComplete({});
		return;
	}

	auto AreRequestedActorClassesLoaded = [this, &PartyIds]() -> bool
	{
		for (const FName& ID : PartyIds)
		{
			const FCharacterSpawnEntry* Entry = SpawnEntryMap.Find(ID);
			if (!Entry || Entry->ActorClass.IsNull() || !Entry->ActorClass.Get())
			{
				return false;
			}
		}
		return true;
	};

	TWeakObjectPtr<UPartyActorSpawnSubsystem> WeakThis(this);

	//TArray<ACombatCharacterActor*> SpawnedActors가 만들어지는 곳이 여기임.(람다)
	auto DoSpawn = [WeakThis, PartyIds, FieldTransforms, OnComplete]()
	{
		UPartyActorSpawnSubsystem* Self = WeakThis.Get();
		if (!Self)
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 에러 - 스폰 완료 전 Subsystem 소멸."));
			return;
		}

		UWorld* World = Self->GetWorld();
		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 에러 - 스폰 완료 시 World 없음."));
			return;
		}

		UCharacterRuntimeSubsystem* CharacterRuntime = nullptr;
		if (UGameInstance* GI = World->GetGameInstance())
			CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>();

		TArray<ACombatCharacterActor*> SpawnedActors;
		SpawnedActors.Reserve(PartyIds.Num());

		const FTransform* LeaderTransform = PartyIds.Num() > 0 ? FieldTransforms.Find(PartyIds[0]) : nullptr;

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
			const FVector SpawnLocation = LeaderLocation - (Direction * 200.0f);
			return FTransform(LeaderTransform->GetRotation(), SpawnLocation);
		};

		for (const FName& ID : PartyIds)
		{
			const FCharacterSpawnEntry* Entry = Self->SpawnEntryMap.Find(ID);
			if (!Entry)
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 에러 - SpawnEntry 없음. ID: %s"), *ID.ToString());
				continue;
			}

			TSubclassOf<ACombatCharacterActor> LoadedClass =
				TSoftClassPtr<ACombatCharacterActor>(Entry->ActorClass.ToSoftObjectPath()).Get();
			if (!LoadedClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 에러 - 클래스 로드 실패. ID: %s"), *ID.ToString());
				continue;
			}

			// 필드 폰 위치가 있으면 그 위치에, 없으면 폴백
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

			Self->SpawnedActorMap.Add(ID, SpawnedActor);
			SpawnedActors.Add(SpawnedActor);

			if (CharacterRuntime)
			{
				// 새 인카운터의 스폰 위치는 FieldTransforms를 우선한다.
				CharacterRuntime->RestoreSnapshot(ID, SpawnedActor, false);
			}

			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 필드 위치 기반 스폰 성공 - %s"), *ID.ToString());
		}

		if (OnComplete) OnComplete(SpawnedActors); // 여기서 람다 호출함.
	};

	if (AreRequestedActorClassesLoaded())
	{
		DoSpawn();
		return;
	}

	TArray<FSoftObjectPath> Paths = CollectSoftPaths(PartyIds);
	if (Paths.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::AsyncSpawnCombatActorsAtFieldPositions : 로드할 경로가 없어 스폰을 시도합니다."));
		DoSpawn();
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	if (PreloadHandle.IsValid())
		PreloadHandle->ReleaseHandle();

	PreloadHandle = StreamableManager.RequestAsyncLoad(
		Paths,
		[DoSpawn]() { DoSpawn(); },
		FStreamableManager::AsyncLoadHighPriority
	);
}

// ============= CombatCharacterActor 파괴 ===============

void UPartyActorSpawnSubsystem::DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors)
{
	UCharacterRuntimeSubsystem* CharacterRuntime = nullptr;
	UWorld* World = GetWorld();
	if (World)
		if (UGameInstance* GI = World->GetGameInstance())
			CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>();

	TSet<ACombatCharacterActor*> ActorsToDespawn;
	for (ACombatCharacterActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			ActorsToDespawn.Add(Actor);
		}
	}

	for (auto It = SpawnedActorMap.CreateIterator(); It; ++It)
	{
		if (ACombatCharacterActor* Actor = It.Value().Get())
		{
			ActorsToDespawn.Add(Actor);
		}
		else
		{
			It.RemoveCurrent();
		}
	}

	if (World)
	{
		for (TActorIterator<ACombatCharacterActor> It(World); It; ++It)
		{
			ACombatCharacterActor* Actor = *It;
			if (IsValid(Actor) && Actor->GetCombatTeam() == ECombatTeam::Player)
			{
				ActorsToDespawn.Add(Actor);
			}
		}
	}

	int32 DespawnedCount = 0;
	for (ACombatCharacterActor* Actor : ActorsToDespawn)
	{
		if (!IsValid(Actor)) continue;

		if (World)
		{
			World->GetTimerManager().ClearAllTimersForObject(Actor);
		}

		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		Actor->SetActorTickEnabled(false);

		// AI 컨트롤러가 있으면 먼저 제거
		if (AController* C = Actor->GetController())
		{
			if (!C->IsPlayerController())
			{
				C->UnPossess();
				C->Destroy();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("DespawnCombatActors : PlayerController 빙의 중 Destroy 시도 - %s"), *Actor->GetName());
				C->UnPossess();
			}
		}
		
		// SpawnedActorMap에서 제거
		for (auto It = SpawnedActorMap.CreateIterator(); It; ++It)
		{
			if (It.Value() == Actor)
			{
				It.RemoveCurrent();
				break;
			}
		}

		// CombatCharacterActor 파괴 직전에 HP/AP/SP 저장
		if (CharacterRuntime)
		{
			const FName CharID = Actor->GetCombatantId();
			if (!CharID.IsNone())
				CharacterRuntime->SaveSnapshot(CharID, Actor);
		}

		Actor->Destroy();
		++DespawnedCount;
	}

	for (auto It = SpawnedActorMap.CreateIterator(); It; ++It)
	{
		if (!It.Value().Get())
		{
			It.RemoveCurrent();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : %d개 액터 디스폰 완료."), DespawnedCount);
}

// ========== 액터 및 캐릭터 ID 조회 ============

TArray<ACombatCharacterActor*> UPartyActorSpawnSubsystem::GetSpawnedActors() const
{
	TArray<ACombatCharacterActor*> Result;
	for (auto& Pair : SpawnedActorMap)
		if (Pair.Value.Get()) Result.Add(Pair.Value.Get());
	return Result;
}

ACombatCharacterActor* UPartyActorSpawnSubsystem::FindActorByCharacterID(const FName& CharacterID) const
{
	if (const TObjectPtr<ACombatCharacterActor>* Found = SpawnedActorMap.Find(CharacterID))
		return Found->Get();
	return nullptr;
}



// ====== 유틸 ========

ACombatCharacterActor* UPartyActorSpawnSubsystem::SpawnSingleActor(
	TSubclassOf<ACombatCharacterActor> ActorClass, const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	
	if (!World || !ActorClass)
		return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

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
				Paths.AddUnique(Entry->ActorClass.ToSoftObjectPath());
			else
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::CollectSoftPaths : %s의 ActorClass가 null."), *ID.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::CollectSoftPaths : 매핑 없음 - %s"), *ID.ToString());
		}
	}

	return Paths;
}
