// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/StreamableManager.h"
#include "PartyActorSpawnSubsystem.generated.h"


class ACombatCharacterActor;

USTRUCT(BlueprintType)
struct FCharacterSpawnEntry

{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName CharacterID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<ACombatCharacterActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector SpawnOffset = FVector::ZeroVector;

};

/**
 * 
 */
UCLASS()
class JRPG_API UPartyActorSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION()
	void RegisterSpawnEntry(const FCharacterSpawnEntry& Entry);

	UFUNCTION()
	void RegisterSpawnEntries(const TArray<FCharacterSpawnEntry>& Entries);


	void PreloadAssets(const TArray<FName>& PartyIds);


	// 비동기 스폰 — 인카운터 연출 시작과 동시에 호출
	void AsyncSpawnCombatActors(
		const TArray<FName>& PartyIds,
		TFunction<void(TArray<ACombatCharacterActor*>)> OnComplete
	);

	// 전투 종료 시 일괄 Destroy
	void DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors);

	// 파티 변경 시 재스폰 트리거 (전투 중이 아닐 때만)
	void OnPartyChanged(const TArray<FName>& NewIds);


	UFUNCTION()
	TArray<ACombatCharacterActor*> GetSpawnedActors() const;

	UFUNCTION()
	ACombatCharacterActor* FindActorByCharacterID(const FName& CharacterID) const;


private:

	//로드된 놈을 실제로 월드에 스폰처리 
	ACombatCharacterActor* SpawnSingleActor(TSubclassOf<ACombatCharacterActor> ActorClass,const FTransform& SpawnTransform);

	TArray<FSoftObjectPath> CollectSoftPaths(const TArray<FName>& PartyIds) const;

	bool IsInBattle() const;

private:

	UPROPERTY()
	TMap<FName, FCharacterSpawnEntry> SpawnEntryMap;

	UPROPERTY()
	TMap<FName, TObjectPtr<ACombatCharacterActor>> SpawnedActorMap;

	//Streamable 핸들 — 사전 로드 유지용 (GC 방지)
	TSharedPtr<FStreamableHandle> PreloadHandle;

	TSet<FName> PendingSpawnIds;	//중복 스폰방지

	
};
