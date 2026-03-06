// Source/JRPGCombat/Public/Combat/Exploration/ExplorationTypes.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ExplorationTypes.generated.h"

// 문서: Type : Chest / Harvest / Landmark / Puzzle / Door / FastTravel / Lore :contentReference[oaicite:7]{index=7}
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

// 문서: TriggerType : Discovery / Interact / Solve / Defeat / Challenge :contentReference[oaicite:8]{index=8}
UENUM()
enum class EExplorationTriggerType : uint8
{
	Discovery,
	Interact,
	Solve,
	Defeat,
	Challenge
};

// 문서: AcquisitionPolicy : OneTime / Respawn :contentReference[oaicite:9]{index=9}
UENUM()
enum class EExplorationAcquisitionPolicy : uint8
{
	OneTime,
	Respawn
};

// 문서: LockCondition : KeyItem / Flag / QuestState / None :contentReference[oaicite:10]{index=10}
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

	UPROPERTY() bool bOk = false;
	UPROPERTY() EExplorationOpResult Result = EExplorationOpResult::Rejected;
	UPROPERTY() FName ReasonTag = NAME_None;

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

	UPROPERTY(EditAnywhere) EExplorationLockType LockType = EExplorationLockType::None;

	// KeyItem용: ItemId (아이템 시스템의 ItemId)
	UPROPERTY(EditAnywhere) FName RequiredItemId = NAME_None;

	// Flag용: FlagId
	UPROPERTY(EditAnywhere) FName RequiredFlagId = NAME_None;

	// QuestState용: 확장 슬롯(추후)
	UPROPERTY(EditAnywhere) FName QuestId = NAME_None;
	UPROPERTY(EditAnywhere) int32 RequiredQuestState = 0;
};

USTRUCT()
struct FDiscoveryRecord
{
	GENERATED_BODY()

	UPROPERTY() bool bDiscovered = false;
	UPROPERTY() double DiscoveredAtRealTime = 0.0;

	// 문서에 OptionalRewardGranted가 언급되므로 슬롯 확보
	UPROPERTY() bool bOptionalRewardGranted = false;
};

// 문서 이벤트 시그니처 :contentReference[oaicite:11]{index=11}
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDiscoveryChanged, FName /*DiscoveryId*/, bool /*bDiscovered*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInteractPromptChanged, FGuid /*ObjectId*/, bool /*bVisible*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnExplorationObjectCompleted, FGuid /*ObjectId*/, EExplorationOpResult /*Result*/, FName /*Reason*/);