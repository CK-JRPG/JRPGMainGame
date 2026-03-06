#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Combat/Exploration/ExplorationTypes.h"
#include "ExplorationRewardTypes.generated.h"

// 아이템/골드/해금/수집/플래그(+EXP 확장)
UENUM()
enum class EExplorationRewardType : uint8
{
	Gold,
	
	// Item 계열 -> InventorySubsystem로 통일
	Material,
	Consumable,
	Equipment,
	KeyItem,

	// Unlock/Collect -> ProgressSubsystem
	MapReveal,
	FastTravelNode,
	TraversalUnlock,
	Collectible,
	Lore,
	Bestiary,

	// 퍼즐키/플래그
	PuzzleKey,
	Flag,

	// 레벨업 시스템이 붙으면 소비(없으면 이벤트로 전달만)
	ExploreExp
};

USTRUCT()
struct FRewardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EExplorationRewardType RewardType = EExplorationRewardType::Gold;
	UPROPERTY(EditAnywhere)
	FName Id = NAME_None;
	UPROPERTY(EditAnywhere)
	int32 Amount = 1;
	UPROPERTY(EditAnywhere)
	float Chance = 1.0f; // 0..1
	UPROPERTY(EditAnywhere)
	bool bUnique = false; // UniqueRewardClaims로 중복 방지
};

USTRUCT()
struct FGrantedReward
{
	GENERATED_BODY()

	UPROPERTY()
	EExplorationRewardType RewardType = EExplorationRewardType::Gold;
	UPROPERTY()
	FName Id = NAME_None;
	UPROPERTY()
	int32 Amount = 0;

	UPROPERTY()
	bool bGranted = false;
	UPROPERTY()
	FName ReasonTag = NAME_None;
};

USTRUCT()
struct FRewardGrantRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid SourceObjectId;
	UPROPERTY()
	FName SourceDiscoveryId = NAME_None;

	UPROPERTY()
	FName SourceTag = "Explore.Reward";
	UPROPERTY()
	EExplorationTriggerType TriggerType = EExplorationTriggerType::Interact;

	UPROPERTY()
	bool bOneTimeContext = false;

	UPROPERTY()
	TArray<FRewardEntry> DirectEntries;
	UPROPERTY()
	TObjectPtr<class UExplorationRewardTableAsset> Table = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AActor> Instigator = nullptr;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRewardsGranted, FGuid /*SourceObjectId*/, const TArray<FGrantedReward>& /*Granted*/);
