// Source/JRPGCombat/Public/Combat/Progression/Leveling/ExpSettingsDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ExpSettingsDataAsset.generated.h"

// 5.1 Travel EXP 규칙/안티익스플로잇 (SSOT) :contentReference[oaicite:13]{index=13}
UCLASS()
class JRPG_API UExpSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---- Level range (문서: LevelMin=1, LevelMax TBD) :contentReference[oaicite:14]{index=14}
	UPROPERTY(EditAnywhere)
	int32 LevelMin = 1;
	UPROPERTY(EditAnywhere)
	int32 LevelMax = 30;

	// ---- Source base values (밸런스는 데이터)
	UPROPERTY(EditAnywhere)
	int32 TravelBaseExp = 2; // 90초/300m당
	UPROPERTY(EditAnywhere)
	int32 DiscoverAreaBaseExp = 30; // 1회성
	UPROPERTY(EditAnywhere)
	int32 DiscoverRestPointBaseExp = 30;
	UPROPERTY(EditAnywhere)
	int32 ExploreRewardBaseExp = 20; // 상자/퍼즐 1회성
	UPROPERTY(EditAnywhere)
	int32 CombatRewardBaseExp = 25; // Victory 기본
	UPROPERTY(EditAnywhere)
	int32 BondPairLevelUpBaseExp = 10; // 페어 레벨업(소량)
	UPROPERTY(EditAnywhere)
	int32 BondTrioLevelUpBaseExp = 30; // 트리오(중요 이벤트)

	// ---- Travel gating (time OR distance)
	UPROPERTY(EditAnywhere)
	float TravelTickSec = 90.f; // 예시: 90초마다 :contentReference[oaicite:15]{index=15}
	UPROPERTY(EditAnywhere)
	float TravelDistanceCm = 30000.f; // 예시: 300m마다 :contentReference[oaicite:16]{index=16}

	// ---- Anti-exploit
	UPROPERTY(EditAnywhere)
	float SamePosRadiusCm = 600.f; // 같은 반경 내 반복 억제 :contentReference[oaicite:17]{index=17}
	UPROPERTY(EditAnywhere)
	int32 PathHistorySize = 12; // 최근 경로 해시/반복 감지 :contentReference[oaicite:18]{index=18}
	UPROPERTY(EditAnywhere)
	float GridQuantizeCm = 500.f; // 위치 셀 해시 그리드
	UPROPERTY(EditAnywhere)
	float BacktrackPenaltyMul = 0.25f; // A-B-A 같은 패턴 감쇠

	// 최근 N초 동안 의미있는 사건 없으면 지급률 감소 :contentReference[oaicite:19]{index=19}
	UPROPERTY(EditAnywhere)
	float InactivityWindowSec = 60.f;
	UPROPERTY(EditAnywhere)
	float InactivityMul = 0.35f;
};
