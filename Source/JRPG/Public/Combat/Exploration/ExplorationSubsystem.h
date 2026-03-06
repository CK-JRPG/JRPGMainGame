// Source/JRPGCombat/Public/Combat/Exploration/ExplorationSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/Exploration/ExplorationTypes.h"
#include "Combat/Exploration/ExplorationRewardTypes.h"

#include "ExplorationSubsystem.generated.h"

class UExplorationObjectDataAsset;
class URewardServiceSubsystem;
class UExplorationSaveGameSubsystem;
class UExplorationProgressSubsystem;
class UExplorationRewardTableAsset;

class AExplorationObjectActor;

UCLASS()
class JRPG_API UExplorationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 문서 이벤트 시그니처 :contentReference[oaicite:28]{index=28}
	FOnDiscoveryChanged OnDiscoveryChanged;
	FOnInteractPromptChanged OnInteractPromptChanged;
	FOnExplorationObjectCompleted OnExplorationObjectCompleted;
	FOnRewardsGranted OnRewardsGranted;

	// ---- Registration ----
	void RegisterObject(AExplorationObjectActor* Obj);
	void UnregisterObject(AExplorationObjectActor* Obj);

	// ---- Query ----
	bool TryGetObjectActor(const FGuid& ObjectId, TWeakObjectPtr<AExplorationObjectActor>& Out) const;
	EExplorationObjectState GetObjectState(const FGuid& ObjectId) const;

	void GetAllRegisteredObjects(TArray<TWeakObjectPtr<AExplorationObjectActor>>& Out) const;

	// ---- Discovery ----
	FExplorationOp TryDiscover(FName DiscoveryId, AActor* Discoverer, UExplorationRewardTableAsset* OptionalRewardTable);

	// ---- Interact/Solve/Defeat/Challenge ----
	FExplorationOp TryInteract(AActor* Interactor, const FGuid& ObjectId);
	FExplorationOp NotifySolved(AActor* Instigator, const FGuid& ObjectId);
	FExplorationOp NotifyDefeated(AActor* Instigator, const FGuid& ObjectId);

	// WorldInteractComponent가 프롬프트 변화 알려줄 때 사용
	void NotifyPromptChanged(const FGuid& ObjectId, bool bVisible);

protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY()
	TMap<FGuid, TWeakObjectPtr<AExplorationObjectActor>> ObjectMap;
	UPROPERTY()
	TMap<FGuid, double> RespawnAvailableAtReal; // runtime cache (save mirror)

	FTimerHandle RespawnTickHandle;

	URewardServiceSubsystem* GetRewardService() const;
	UExplorationSaveGameSubsystem* GetSaveSys() const;
	UExplorationProgressSubsystem* GetProgressSys() const;

	double NowReal() const;
	void TickRespawns();

	// lock check
	FExplorationOp CheckLock(const UExplorationObjectDataAsset& Data) const;

	// complete handling
	FExplorationOp CompleteObject(AActor* Instigator, const UExplorationObjectDataAsset& Data,
	                              EExplorationTriggerType TriggerType);

	// state sync
	void ApplyInitialStateToActor(AExplorationObjectActor* Obj, const UExplorationObjectDataAsset& Data);
};
