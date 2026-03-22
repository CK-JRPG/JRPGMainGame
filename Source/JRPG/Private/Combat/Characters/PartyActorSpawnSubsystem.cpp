// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Characters/PartyActorSpawnSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Engine/AssetManager.h"

void UPartyActorSpawnSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	SpawnEntryMap.Empty();
	SpawnedActorMap.Empty();

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 초기화 완료."));
}

void UPartyActorSpawnSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	TArray<TObjectPtr<ACombatCharacterActor>> Remaining;
	SpawnedActorMap.GenerateValueArray(Remaining);
	DespawnCombatActors(Remaining);
}

void UPartyActorSpawnSubsystem::RegisterSpawnEntry(const FCharacterSpawnEntry& Entry)
{
	if (Entry.CharacterID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::RegisterSpawnEntry : 실패 - CharacterID가 None임."));
		return;
	}

	SpawnEntryMap.Add(Entry.CharacterID, Entry);
	UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::RegisterSpawnEntry :  등록: %s"), *Entry.CharacterID.ToString());

}

void UPartyActorSpawnSubsystem::RegisterSpawnEntries(const TArray<FCharacterSpawnEntry>& Entries)
{
	for (const FCharacterSpawnEntry& Entry : Entries)
	{
		RegisterSpawnEntry(Entry);
	}
}

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
			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnerSubsystem : 에셋 사전 로드 완료."))
		}, FStreamableManager::AsyncLoadHighPriority);
}

void UPartyActorSpawnSubsystem::AsyncSpawnCombatActors(const TArray<FName>& PartyIds,  const FTransform& SpawnOrigin,
	TFunction<void(TArray<ACombatCharacterActor*>)> OnComplete)
{
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnerSubsystem : AsyncSpawnCombatActors: PartyIds가 비어 있음."));
	
		if (OnComplete)
			OnComplete({});
		return;
	}

	TArray<FSoftObjectPath> Paths = CollectSoftPaths(PartyIds);
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	//람다 캡처 
	TWeakObjectPtr<UPartyActorSpawnSubsystem> WeakThis(this);
	
	StreamableManager.RequestAsyncLoad(Paths, [WeakThis, PartyIds, SpawnOrigin, OnComplete]()
	{
		UPartyActorSpawnSubsystem* Self = WeakThis.Get();
		if (!Self)
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnerSubsystem : 에러 - 스폰 완료 전 Subsystem 소멸."));
			return;
		}
		
		UWorld* World = Self->GetWorld();
		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnerSubsystem : 에러 - 스폰 완료 시에 World가 없음."));
			return;
		}
		
		TArray<ACombatCharacterActor*> SpawnedActors;
		SpawnedActors.Reserve(PartyIds.Num());
		
		for (int32 i = 0; i < PartyIds.Num(); ++i)
		{
			const FName& ID = PartyIds[i];
			const FCharacterSpawnEntry* Entry = Self -> SpawnEntryMap.Find(ID);
			if (!Entry)
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnerSubsystem : 에러 - ID에 해당하는 SpawnEntry가 없음. ID: %s"), *ID.ToString());
				continue;
			}
			
			//로드된 클래스 가지고 오기
			TSubclassOf<ACombatCharacterActor> LoadedClass = TSoftClassPtr<ACombatCharacterActor>(Entry->ActorClass.ToSoftObjectPath()).Get();
			if (!LoadedClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnerSubsystem : 에러 - ID에 해당하는 클래스 로드 실패. ID: %s"), *ID.ToString());
				continue;
			}
			
			FTransform ActorTransform = SpawnOrigin;
			ActorTransform.AddToTranslation(Entry->SpawnOffset);
			
			ACombatCharacterActor* SpawnedActor = Self->SpawnSingleActor(LoadedClass, ActorTransform);
			if (SpawnedActor)
			{
				Self->SpawnedActorMap.Add(ID, SpawnedActor);
				SpawnedActors.Add(SpawnedActor);
				
				UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnerSubsystem : 스폰 성공 - ID: %s"), *ID.ToString());
			}
		}
		
		if (OnComplete)
		{
			OnComplete(SpawnedActors);
		}
	
		
	}, FStreamableManager::AsyncLoadHighPriority);

}

void UPartyActorSpawnSubsystem::DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors)
{
	for (ACombatCharacterActor* Actor : Actors)
	{
		if (!IsValid(Actor))
			continue;
		
		for (auto It = SpawnedActorMap.CreateIterator(); It; ++It)
		{
			if (It.Value() == Actor)
			{
				It.RemoveCurrent();
				break;
			}
		}
		
		Actor->Destroy();
	}
	
	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnerSubsystem : $d개 액터 디스폰 완료."), Actors.Num());
}

void UPartyActorSpawnSubsystem::SetOriginalPlayerCharacterID(const FName& CharacterID)
{
	OriginalPlayerCharacterID = CharacterID;
	CurrentPlayerCharacterID  = CharacterID;

	UE_LOG(LogTemp, Log,
		TEXT("PartyActorSpawnerSubsystem : 원본 플레이어 캐릭터 설정: %s"),
		*CharacterID.ToString());
}

void UPartyActorSpawnSubsystem::SetCombatPlayerController(APlayerController* InController)
{
	CombatPlayerController = InController;
}


void UPartyActorSpawnSubsystem::OnPartyChanged(const FName& NewCharacterID)
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	if (!BattleSession->IsBattleActive())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PartyActorSpawnerSubsystem : 필드 상태에서 조작 캐릭터 전환 불가."));
		return;
	}

	if (CurrentPlayerCharacterID == NewCharacterID) return;

	ACombatCharacterActor* TargetActor = FindActorByCharacterID(NewCharacterID);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Error,
			TEXT("PartyActorSpawnerSubsystem : 전환 대상 Actor 없음: %s"), *NewCharacterID.ToString());
		return;
	}

	if (!CombatPlayerController)
	{
		UE_LOG(LogTemp, Error,
			TEXT("PartyActorSpawnerSubsystem : CombatPlayerController 미등록. SetCombatPlayerController 호출 여부 확인."));
		return;
	}

	// 기존 빙의 해제 후 새 Actor에 빙의
	CombatPlayerController->UnPossess();
	CombatPlayerController->Possess(TargetActor);

	CurrentPlayerCharacterID = NewCharacterID;

	UE_LOG(LogTemp, Log,
		TEXT("PartyActorSpawnerSubsystem : 빙의 전환 완료 → %s"), *NewCharacterID.ToString());
}

void UPartyActorSpawnSubsystem::OnBattleEnded()
{
	if (OriginalPlayerCharacterID.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnerSubsystem : OnBattleEnded : OriginalPlayerCharacterID 미설정"));
		return;
	}
	
	if (CurrentPlayerCharacterID != OriginalPlayerCharacterID)
	{
		ACombatCharacterActor* OriginalActor = FindActorByCharacterID(OriginalPlayerCharacterID);
		if (OriginalActor && CombatPlayerController)
		{
			CombatPlayerController->UnPossess();
			CombatPlayerController->Possess(OriginalActor);
			
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnerSubsystem :  빙의 복구 - %s → %s"),
				*CurrentPlayerCharacterID.ToString(),
				*OriginalPlayerCharacterID.ToString());
		}
	}
	
	TArray<TObjectPtr<ACombatCharacterActor>> CurrentActors;
	SpawnedActorMap.GenerateValueArray(CurrentActors);
	DespawnCombatActors(CurrentActors);
	
	CombatPlayerController = nullptr;
	OriginalPlayerCharacterID = NAME_None;
	CurrentPlayerCharacterID  = NAME_None;
	
	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnerSubsystem : 전투 종료 처리 완료."));

}


TArray<ACombatCharacterActor*> UPartyActorSpawnSubsystem::GetSpawnedActors() const
{
	TArray<TObjectPtr<ACombatCharacterActor>> Result;
	SpawnedActorMap.GenerateValueArray(Result);
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

ACombatCharacterActor* UPartyActorSpawnSubsystem::SpawnSingleActor(TSubclassOf<ACombatCharacterActor> ActorClass,
	const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

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
			{
				Paths.AddUnique(Entry->ActorClass.ToSoftObjectPath());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnerSubsystem : CollectSoftPaths: %s의 ActorClass가 null."), *ID.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnerSubsystem : CollectSoftPaths: 매핑 없음 — %s"), *ID.ToString());
		}
	}
	return Paths;

}

