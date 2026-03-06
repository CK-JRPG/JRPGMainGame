// Source/JRPGCombat/Public/Combat/Exploration/ExplorationRewardTypes.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ExplorationRewardTypes.generated.h"

// 문서: RewardType (Gold/Material/Equipment/Collectible/Unlock/KeyItem 등) :contentReference[oaicite:12]{index=12}
// 문서: 탐험 보상 카테고리 SSOT :contentReference[oaicite:13]{index=13}
UENUM()
enum class EExplorationRewardType : uint8
{
	// 경제/자원
	Gold,
	CraftMaterial,
	Consumable,

	// 성장/전투 강화
	Equipment,
	SkillResource,

	// 탐험 해금
	MapReveal,
	FastTravelNode,
	TraversalUnlock,

	// 수집/도감
	Collectible,
	Lore,
	Bestiary,

	// 시스템 키
	KeyItem,
	PuzzleKey,// 보통 Flag로 처리

	// 확장: 경험치(레벨업 시스템은 OnRewardsGranted에서 트리거) :contentReference[oaicite:14]{index=14}
	ExploreExp,

	// 확장: 상점/경제는 별도 문서이므로 여기선 Flag/Unlock으로만 이어 붙이는 걸 권장 :contentReference[oaicite:15]{index=15}
	Flag
};

USTRUCT()
struct FRewardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EExplorationRewardType RewardType = EExplorationRewardType::Gold;

	// 문서: Id(아이템ID/노드ID) :contentReference[oaicite:16]{index=16}
	UPROPERTY(EditAnywhere) FName Id = NAME_None;

	// 문서: Amount :contentReference[oaicite:17]{index=17}
	UPROPERTY(EditAnywhere) int32 Amount = 1;

	// 문서: Chance (드랍/랜덤) :contentReference[oaicite:18]{index=18}
	UPROPERTY(EditAnywhere) float Chance = 1.0f; // 0..1

	// 문서: bUnique(중복 방지) :contentReference[oaicite:19]{index=19}
	UPROPERTY(EditAnywhere) bool bUnique = false;
};

USTRUCT()
struct FGrantedReward
{
	GENERATED_BODY()

	UPROPERTY() EExplorationRewardType RewardType = EExplorationRewardType::Gold;
	UPROPERTY() FName Id = NAME_None;
	UPROPERTY() int32 Amount = 0;
	UPROPERTY() bool bGranted = false;
	UPROPERTY() FName ReasonTag = NAME_None;
};

USTRUCT()
struct FRewardGrantRequest
{
	GENERATED_BODY()

	// 어떤 오브젝트/발견/전투에서 왔는지
	UPROPERTY() FGuid SourceObjectId;
	UPROPERTY() FName SourceDiscoveryId = NAME_None;

	// 문서 텔레메트리 source(Explore.RewardGranted)와 연결될 SourceTag :contentReference[oaicite:20]{index=20}
	UPROPERTY() FName SourceTag = "Explore.Reward";

	UPROPERTY() EExplorationTriggerType TriggerType = EExplorationTriggerType::Interact;

	// OneTime/Discovery 최초 보상에 주로 사용
	UPROPERTY() bool bOneTimeContext = false;

	UPROPERTY() TArray<FRewardEntry> DirectEntries; // DA가 직접 엔트리를 들고 있는 경우
	UPROPERTY() TObjectPtr<class UExplorationRewardTableAsset> Table = nullptr; // 테이블 기반

	UPROPERTY() TWeakObjectPtr<AActor> Instigator = nullptr;
};

// 문서 이벤트: OnRewardsGranted(SourceId, RewardList) :contentReference[oaicite:21]{index=21}
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRewardsGranted, FGuid /*SourceObjectId*/, const TArray<FGrantedReward>& /*Granted*/);