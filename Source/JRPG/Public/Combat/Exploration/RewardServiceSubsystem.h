#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Exploration/ExplorationRewardTypes.h"
#include "RewardServiceSubsystem.generated.h"

// (확장) 레벨업 시스템이 이 인터페이스를 구현하면 ExploreExp를 여기서 바로 소비 가능
UINTERFACE(MinimalAPI)
class UCombatExpMutator : public UInterface
{
	GENERATED_BODY()
};

class ICombatExpMutator
{
	GENERATED_BODY()

public:
	virtual bool GrantExploreExp(int32 BaseExp, FGuid ContextId, FName SourceTag, FName& OutReason) = 0;
};

UCLASS()
class JRPG_API URewardServiceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FExplorationOp GrantRewards(const FRewardGrantRequest& Req, /*out*/ TArray<FGrantedReward>& OutGranted);

private:
	bool RollChance(float Chance01) const;

	uint64 MakeUniqueKey(const FRewardGrantRequest& Req, const FRewardEntry& E) const;
	FGuid MakeExpContext(const FRewardGrantRequest& Req, const FRewardEntry& E) const;

	FGrantedReward ApplyOne(const FRewardGrantRequest& Req, const FRewardEntry& E);

	class UInventorySubsystem* GetInventory() const;
	class UEconomySubsystem* GetEconomy() const;
	class UExplorationSaveGameSubsystem* GetExploreSave() const;
	class UExplorationProgressSubsystem* GetExploreProgress() const;

	ICombatExpMutator* FindExpMutator(FName& OutReason) const;
};
