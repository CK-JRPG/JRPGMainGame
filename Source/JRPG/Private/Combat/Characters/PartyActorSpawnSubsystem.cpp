// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Characters/PartyActorSpawnSubsystem.h"

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
			UE_LOG(LogTemp, Log, TEXT("[PartyActorSpawnerSubsystem] 에셋 사전 로드 완료."))
		}, FStreamableManager::AsyncLoadHighPriority);
}

void UPartyActorSpawnSubsystem::AsyncSpawnCombatActors(const TArray<FName>& PartyIds,
	TFunction<void(TArray<ACombatCharacterActor*>)> OnComplete)
{
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartyActorSpawnerSubsystem] AsyncSpawnCombatActors: PartyIds가 비어 있음."));
	
		if (OnComplete)
			OnComplete({});
		return;
	}

	TArray<FSoftObjectPath> Paths = CollectSoftPaths(PartyIds);
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	//람다 캡처 
	TWeakObjectPtr<UPartyActorSpawnSubsystem> WeakThis(this);


}

void UPartyActorSpawnSubsystem::DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors)
{
}

void UPartyActorSpawnSubsystem::OnPartyChanged(const TArray<FName>& NewIds)
{
}

TArray<ACombatCharacterActor*> UPartyActorSpawnSubsystem::GetSpawnedActors() const
{
}

ACombatCharacterActor* UPartyActorSpawnSubsystem::FindActorByCharacterID(const FName& CharacterID) const
{
}

ACombatCharacterActor* UPartyActorSpawnSubsystem::SpawnSingleActor(TSubclassOf<ACombatCharacterActor> ActorClass,
	const FTransform& SpawnTransform)
{
}

TArray<FSoftObjectPath> UPartyActorSpawnSubsystem::CollectSoftPaths(const TArray<FName>& PartyIds) const
{
}

bool UPartyActorSpawnSubsystem::IsInBattle() const
{
}
