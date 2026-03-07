#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ExplorationTypes.generated.h"

UENUM()
enum class EExplorationObjectType : uint8
{
	Chest,
	Harvest,
	Landmark,
	Puzzle,
	Door,
	FastTravel,
	Lore
};

UENUM()
enum class EExplorationTriggerType : uint8
{
	Discovery,
	Interact,
	Solve,
	Defeat,
	Challenge
};

UENUM()
enum class EExplorationAcquisitionPolicy : uint8
{
	OneTime,
	Respawn
};

UENUM()
enum class EExplorationLockType : uint8
{
	None,
	KeyItem,
	Flag,
	QuestState
};

UENUM()
enum class EExplorationObjectState : uint8
{
	Active,
	CompletedOneTime,
	WaitingRespawn
};

UENUM()
enum class EExplorationOpResult : uint8
{
	Success,
	Rejected
};

USTRUCT()
struct FExplorationOp
{
	GENERATED_BODY()

	UPROPERTY()
	bool bOk = false;
	UPROPERTY()
	EExplorationOpResult Result = EExplorationOpResult::Rejected;
	UPROPERTY()
	FName ReasonTag = NAME_None;

	static FExplorationOp Ok()
	{
		FExplorationOp O;
		O.bOk = true;
		O.Result = EExplorationOpResult::Success;
		return O;
	}

	static FExplorationOp Fail(FName Reason)
	{
		FExplorationOp O;
		O.bOk = false;
		O.Result = EExplorationOpResult::Rejected;
		O.ReasonTag = Reason;
		return O;
	}
};

USTRUCT()
struct FExplorationLockCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EExplorationLockType LockType = EExplorationLockType::None;

	// KeyItem용(아이템 시스템 ItemId)
	UPROPERTY(EditAnywhere)
	FName RequiredItemId = NAME_None;

	// Flag용(탐험 진행 플래그)
	UPROPERTY(EditAnywhere)
	FName RequiredFlagId = NAME_None;

	// QuestState용(추후 확장)
	UPROPERTY(EditAnywhere)
	FName QuestId = NAME_None;
	UPROPERTY(EditAnywhere)
	int32 RequiredQuestState = 0;
};

USTRUCT()
struct FDiscoveryRecord
{
	GENERATED_BODY()

	UPROPERTY()
	bool bDiscovered = false;
	UPROPERTY()
	double DiscoveredAtRealTime = 0.0;
	UPROPERTY()
	bool bOptionalRewardGranted = false;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDiscoveryChanged, FName /*DiscoveryId*/, bool /*bDiscovered*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInteractPromptChanged, FGuid /*ObjectId*/, bool /*bVisible*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnExplorationObjectCompleted, FGuid /*ObjectId*/, EExplorationOpResult /*Result*/, FName /*Reason*/);
