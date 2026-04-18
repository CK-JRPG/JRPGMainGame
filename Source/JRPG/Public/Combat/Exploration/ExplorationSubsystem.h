#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/Exploration/ExplorationTypes.h"
#include "Combat/Exploration/ExplorationRewardTypes.h"

#include "ExplorationSubsystem.generated.h"

class UExplorationObjectDataAsset;
class UExplorationSaveGameSubsystem;
class UExplorationProgressSubsystem;
class URewardServiceSubsystem;
class UExplorationRewardTableAsset;

class AExplorationObjectActor;

UCLASS()
class JRPG_API UExplorationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnDiscoveryChanged OnDiscoveryChanged;
	FOnInteractPromptChanged OnInteractPromptChanged;
	FOnExplorationObjectCompleted OnExplorationObjectCompleted;
	FOnRewardsGranted OnRewardsGranted;

	void RegisterObject(AExplorationObjectActor* Obj);
	void UnregisterObject(AExplorationObjectActor* Obj);

	bool TryGetObjectActor(const FGuid& ObjectId, TWeakObjectPtr<AExplorationObjectActor>& Out) const;
	void GetAllRegisteredObjects(TArray<TWeakObjectPtr<AExplorationObjectActor>>& Out) const;

	EExplorationObjectState GetObjectState(const FGuid& ObjectId) const;

	// Discovery
	FExplorationOp TryDiscover(FName DiscoveryId, AActor* Discoverer,
	                           UExplorationRewardTableAsset* OptionalRewardTable);

	// Interact/Solve/Defeat/Challenge
	FExplorationOp TryInteract(AActor* Interactor, const FGuid& ObjectId);
	FExplorationOp NotifySolved(AActor* Instigator, const FGuid& ObjectId);
	FExplorationOp NotifyDefeated(AActor* Instigator, const FGuid& ObjectId);
	FExplorationOp NotifyChallengeCompleted(AActor* Instigator, const FGuid& ObjectId);

	// Prompt (WorldInteractComponent에서 호출)
	void NotifyPromptChanged(const FGuid& ObjectId, bool bVisible);

protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY()
	TMap<FGuid, TWeakObjectPtr<AExplorationObjectActor>> ObjectMap;
	FTimerHandle RespawnTickHandle;

	URewardServiceSubsystem* GetRewardService() const;
	UExplorationSaveGameSubsystem* GetSaveSys() const;
	UExplorationProgressSubsystem* GetProgressSys() const;

	double NowReal() const;
	void TickRespawns();

	FExplorationOp CheckLock(const UExplorationObjectDataAsset& Data) const;

	FExplorationOp CompleteObject(AActor* Instigator, const UExplorationObjectDataAsset& Data,
	                              EExplorationTriggerType TriggerType);

	void ApplyInitialStateToActor(AExplorationObjectActor* Obj, const UExplorationObjectDataAsset& Data);
};
