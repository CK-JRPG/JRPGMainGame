#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/StreamableManager.h"
#include "PartyActorSpawnSubsystem.generated.h"

class ACombatCharacterActor;
class AJRPGCompanionPawn;

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

	UPROPERTY(EditAnywhere)       
	TSoftClassPtr<AJRPGCompanionPawn> FieldPawnClass;
};



// 전투 액터 스폰/파괴 전담 서브시스템
// SpawnEntry 등록
// CombatCharacterActor 비동기 스폰/파괴
// 에셋 프리로드
// 스폰된 액터 조회

UCLASS()
class JRPG_API UPartyActorSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	
	// SpawnEntry 레지스트리
	UFUNCTION()
	void RegisterSpawnEntry(const FCharacterSpawnEntry& Entry);

	UFUNCTION()
	void RegisterSpawnEntries(const TArray<FCharacterSpawnEntry>& Entries);

	const TMap<FName, FCharacterSpawnEntry>& GetSpawnEntryMap() const { return SpawnEntryMap; }

	//에셋 프리로드
	void PreloadAssets(const TArray<FName>& PartyIds);

	// CombatCharacterActor 스폰/파괴 

	// 필드 폰 위치/회전 기반 비동기 스폰.
	// OnComplete는 Session/Transition 준비가 끝나 Actor 소유권을 확정할 때만 true를 반환한다.
	void AsyncSpawnCombatActorsAtFieldPositions(
		const TArray<FName>& PartyIds,
		const TMap<FName, FTransform>& FieldTransforms,
		TFunction<bool(TArray<ACombatCharacterActor*>)> OnComplete
	);

	// 전투 종료 시 Runtime State batch commit 후 CombatCharacterActor 파괴
	void DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors);

	// 조회
	UFUNCTION()
	ACombatCharacterActor* FindActorByCharacterID(const FName& CharacterID) const;

	UFUNCTION()
	TArray<ACombatCharacterActor*> GetSpawnedActors() const;

private:
	ACombatCharacterActor* SpawnSingleActor(TSubclassOf<ACombatCharacterActor> ActorClass, const FTransform& SpawnTransform);
	TArray<FSoftObjectPath> CollectSoftPaths(const TArray<FName>& PartyIds) const;
	void DespawnCombatActorsInternal(const TArray<ACombatCharacterActor*>& Actors, bool bCommitRuntimeState);
	void DiscardCombatActors(const TArray<ACombatCharacterActor*>& Actors);
	void InvalidateSpawnRequest();
	bool HasOwnedCombatActors() const;

private:
	UPROPERTY()
	TMap<FName, FCharacterSpawnEntry> SpawnEntryMap;

	UPROPERTY()
	TMap<FName, TObjectPtr<ACombatCharacterActor>> SpawnedActorMap;

	TSet<FName> PendingSpawnIds;

	TSharedPtr<FStreamableHandle> PreloadHandle;
	TSharedPtr<FStreamableHandle> SpawnLoadHandle;
	uint64 SpawnRequestSerial = 0;
	uint64 ActiveSpawnRequestId = 0;
	bool bSpawnRequestInFlight = false;
};
