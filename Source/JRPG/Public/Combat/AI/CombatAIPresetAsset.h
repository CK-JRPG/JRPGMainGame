// Source/JRPGCombat/Public/Combat/AI/CombatAIPresetAsset.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatAIPresetAsset.generated.h"

UENUM(BlueprintType)
enum class EPartyRole : uint8
{
	Defender,
	Attacker,
	Supporter,
};

UENUM(BlueprintType)
enum class EPartyAIPreset : uint8
{
	Basic,
	Aggressive,
	Defensive,
};

USTRUCT(BlueprintType)
struct FPartyAIThresholds
{
	GENERATED_BODY()

	// AI 문서: Supporter 힐 임계는 프리셋에 따라 변화(예: 30% → 20% / 45%) :contentReference[oaicite:10]{index=10}
	UPROPERTY(EditAnywhere) float SupporterHealCriticalHp01 = 0.30f;

	// “아군 위험(HP<30% 또는 FocusFire)” 시 Defender 보호/도발 우선 :contentReference[oaicite:11]{index=11}
	UPROPERTY(EditAnywhere) float PartyDangerHp01 = 0.30f;

	// Attacker: 브레이크 임계 근접 시 브레이크 스킬 우선 :contentReference[oaicite:12]{index=12}
	UPROPERTY(EditAnywhere) float BreakNearThreshold01 = 0.85f;

	// Rising 중 공격 억제/제한(기본 false) :contentReference[oaicite:13]{index=13}
	UPROPERTY(EditAnywhere) bool bEnemyAttackAllowedWhileRising = false;
};

USTRUCT(BlueprintType)
struct FPartyAIWeights
{
	GENERATED_BODY()

	// 역할 수행 점수(기본)
	UPROPERTY(EditAnywhere) float ThreatHold = 2.0f;// Defender
	UPROPERTY(EditAnywhere) float ProtectAlly = 3.0f;// Defender
	UPROPERTY(EditAnywhere) float BreakBuild = 3.0f;// Attacker
	UPROPERTY(EditAnywhere) float HighDps = 2.0f;// Attacker
	UPROPERTY(EditAnywhere) float HealCritical = 5.0f;// Supporter
	UPROPERTY(EditAnywhere) float Cleanse = 4.0f;// Supporter
	UPROPERTY(EditAnywhere) float BuffUptime = 2.0f;// Supporter

	// SP 잘 버는 행동에 추가 가산 (SP는 롤에 맞는 행동을 보상 :contentReference[oaicite:14]{index=14})
	UPROPERTY(EditAnywhere) float SPBonusMultiplier = 1.0f;
};

UCLASS()
class JRPG_API UCombatAIPresetAsset :public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) EPartyAIPreset Preset = EPartyAIPreset::Basic;

	// AI가 스킬/행동 선택을 갱신하는 주기 (너무 짧으면 스팸)
	UPROPERTY(EditAnywhere) float DecisionIntervalSec = 0.25f;

	UPROPERTY(EditAnywhere) FPartyAIThresholds Thresholds;
	UPROPERTY(EditAnywhere) FPartyAIWeights Weights;

	// Enemy 데이터(문서에 존재): RisingAttackAllowed 등 :contentReference[oaicite:15]{index=15}
	UPROPERTY(EditAnywhere) bool bEnemyRisingAttackAllowed = false;
	UPROPERTY(EditAnywhere) float EnemyRisingTargetLockMultiplier = 2.0f;
};