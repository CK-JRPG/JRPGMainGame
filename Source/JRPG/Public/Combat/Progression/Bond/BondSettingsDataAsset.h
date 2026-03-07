// Source/JRPGCombat/Public/Combat/Progression/Bond/BondSettingsDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Progression/Bond/BondTypes.h"
#include "BondSettingsDataAsset.generated.h"

// 7.3 BP 획득 설정(테이블) + 7.4 보너스 테이블 (SSOT) :contentReference[oaicite:11]{index=11}
UCLASS()
class JRPG_API UBondSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// Walk
	UPROPERTY(EditAnywhere)
	float WalkTickSec = 60.f; // 예시 :contentReference[oaicite:12]{index=12}
	UPROPERTY(EditAnywhere)
	float WalkDistanceCm = 20000.f; // 200m 예시(문서 4.1 예시) :contentReference[oaicite:13]{index=13}
	UPROPERTY(EditAnywhere)
	int32 WalkBPBase = 2; // 예시 :contentReference[oaicite:14]{index=14}

	// Combat/Rest
	UPROPERTY(EditAnywhere)
	int32 CombatWinBPBase = 6; // 예시 :contentReference[oaicite:15]{index=15}
	UPROPERTY(EditAnywhere)
	int32 RestTalkBPBase = 12; // 예시 :contentReference[oaicite:16]{index=16}

	// 감쇠 테이블(레벨이 오를수록 감소) 예시: 1.0,0.85,0.7,0.55,0.0 :contentReference[oaicite:17]{index=17}
	UPROPERTY(EditAnywhere)
	TArray<float> DiminishByLevel = {0.f, 1.0f, 0.85f, 0.70f, 0.55f, 0.0f}; // index=BL(1..5)

	// TrioLevel -> ExpBonusMultiplier (예: 1.00~1.08) :contentReference[oaicite:18]{index=18}
	UPROPERTY(EditAnywhere)
	TArray<float> ExpBonusByTrioLevel = {0.f, 1.00f, 1.02f, 1.04f, 1.06f, 1.08f};

	// Anti-exploit: 동일 bond의 BP 이벤트 쿨다운 (Reason: Reject.AntiExploitCooldown) :contentReference[oaicite:19]{index=19}
	UPROPERTY(EditAnywhere)
	float AntiExploitCooldownSec = 6.f;

	// Walk: 유의미 진행 없으면 획득 속도 감소 :contentReference[oaicite:20]{index=20}
	UPROPERTY(EditAnywhere)
	float InactivityWindowSec = 60.f;
	UPROPERTY(EditAnywhere)
	float InactivityMul = 0.35f;

	// Trio 이벤트를 페어로 분배할지(기본: 분배) - 밸런스 안전장치
	UPROPERTY(EditAnywhere)
	bool bDistributeTrioEventToPairs = true;

	float GetDiminishMul(int32 BondLevel) const
	{
		const int32 L = FMath::Clamp(BondLevel, 1, 5);
		return DiminishByLevel.IsValidIndex(L) ? FMath::Clamp(DiminishByLevel[L], 0.f, 1.f) : 1.0f;
	}

	float GetExpBonusMulByTrioLevel(int32 TrioLevel) const
	{
		const int32 L = FMath::Clamp(TrioLevel, 1, 5);
		return ExpBonusByTrioLevel.IsValidIndex(L) ? FMath::Max(0.f, ExpBonusByTrioLevel[L]) : 1.0f;
	}
};
