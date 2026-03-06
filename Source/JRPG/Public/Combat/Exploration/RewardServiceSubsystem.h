// Source/JRPGCombat/Public/Combat/Exploration/RewardServiceSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Exploration/ExplorationRewardTypes.h"
#include "RewardServiceSubsystem.generated.h"

UCLASS()
class JRPG_API URewardServiceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 실제 지급(Inventory/Economy/Progress 연동)
	FExplorationOp GrantRewards(const FRewardGrantRequest& Req, /*out*/ TArray<FGrantedReward>& OutGranted);

private:
	bool RollChance(float Chance01) const;

	// Unique claim key
	uint64 MakeUniqueKey(const FRewardGrantRequest& Req, const FRewardEntry& E) const;

	// Apply one reward
	FGrantedReward ApplyOne(const FRewardGrantRequest& Req, const FRewardEntry& E);

	// Access services
	class UInventorySubsystem* GetInventory() const;
	class UEconomySubsystem* GetEconomy() const;
	class UExplorationSaveGameSubsystem* GetExploreSave() const;
	class UExplorationProgressSubsystem* GetExploreProgress() const;

	double NowReal() const;
};
