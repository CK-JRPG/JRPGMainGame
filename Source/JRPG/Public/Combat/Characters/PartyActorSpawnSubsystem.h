#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/StreamableManager.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "PartyActorSpawnSubsystem.generated.h"


class ACombatPlayerController;
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


UCLASS()
class JRPG_API UPartyActorSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	
	// 필드 폰 위치/회전 기반 비동기 스폰 
	void AsyncSpawnCombatActorsAtFieldPositions(
		const TArray<FName>& PartyIds,
		const TMap<FName, FTransform>& FieldTransforms,
		TFunction<void(TArray<ACombatCharacterActor*>)> OnComplete
	);

	UFUNCTION()
	void RegisterSpawnEntry(const FCharacterSpawnEntry& Entry);
	

	UFUNCTION()
	void RegisterSpawnEntries(const TArray<FCharacterSpawnEntry>& Entries);
	
	void PreloadAssets(const TArray<FName>& PartyIds);
	
	void SpawnFieldCompanions(const FVector& LeaderLocation, const FName& LeaderCharacterID);
	void DespawnFieldCompanions();
	void OnPartyMemberChanged(const FName& NewCharacterID);

	// 전투 모드 전환시키기 필드 폰 숨기고 CombatCharacterActor 빙의
	void EnterCombatMode(APlayerController* PC, const FName& LeaderCharacterID);
	
	// 전투 종료 시 CombatChracterActor는 모두 파괴함.
	void DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors);
	void OnBattleEnded(EBattleEndReason Reason);

	
	
	//유틸함수
	UFUNCTION()
	ACombatCharacterActor* FindActorByCharacterID(const FName& CharacterID) const;
	
	void SetOriginalPlayerCharacterID(const FName& CharacterID);
	FName GetCurrentPlayerCharacterID() const { return CurrentPlayerCharacterID; }
	
	void SetCombatPlayerController(APlayerController* InController);
	void SetCombatControllerClass(TSubclassOf<APlayerController> InClass);
	
	UFUNCTION()
	TArray<ACombatCharacterActor*> GetSpawnedActors() const;
	
	UFUNCTION()
	TArray<AJRPGCompanionPawn*> GetSpawnedCompanions() const;

private:
	//로드된 놈을 실제로 월드에 스폰처리 
	ACombatCharacterActor* SpawnSingleActor(TSubclassOf<ACombatCharacterActor> ActorClass,const FTransform& SpawnTransform);

	TArray<FSoftObjectPath> CollectSoftPaths(const TArray<FName>& PartyIds) const;
	
	//이동 입력시 속도 동기화
	void SyncMovementStateToLeader(APawn* FieldPawn, ACombatCharacterActor* LeaderActor);
	void SyncMovementStateToFieldPawn(ACombatCharacterActor* LeaderActor, APawn* FieldPawn);
	
private:
	UPROPERTY()
	TObjectPtr<APlayerController> CombatPlayerController;
	
	// BP에서 파생된 CombatPlayerController 클래스 (미설정 시 C++ 기본 클래스 사용)
	UPROPERTY()
	TSubclassOf<APlayerController> CombatControllerClass;

	// 전투 진입 전 필드 컨트롤러 캐시 (전투 종료 후 복원)
	UPROPERTY()
	TObjectPtr<APlayerController> CachedFieldController;
	
	UPROPERTY()
	TMap<FName, TObjectPtr<AJRPGCompanionPawn>> SpawnedCompanionMap;
	
	UPROPERTY()
	TMap<FName, FCharacterSpawnEntry> SpawnEntryMap;

	UPROPERTY()
	TMap<FName, TObjectPtr<ACombatCharacterActor>> SpawnedActorMap;

	TSet<FName> PendingSpawnIds;	//중복 스폰방지

	FName OriginalPlayerCharacterID;
	FName CurrentPlayerCharacterID;

	// 전투 진입 전 필드 폰 캐시용도(전투 끝나면 복구함)
	UPROPERTY()
	TObjectPtr<APawn> CachedFieldPawn;

	// 배틀세션쪽 OnBattleEnded 델리게이트
	void HandleBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);

	//Streamable 핸들
	TSharedPtr<FStreamableHandle> PreloadHandle;
	TSharedPtr<FStreamableHandle> FieldPawnPreloadHandle;
};
