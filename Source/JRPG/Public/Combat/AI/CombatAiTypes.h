// Source/JRPGCombat/Public/Combat/AI/CombatAITypes.h
#pragma once

#include"CoreMinimal.h"
#include"GameplayTagContainer.h"
#include"CombatAITypes.generated.h"

UENUM()
enum class ECombatPartyRole : uint8
{
	Defender UMETA(DisplayName="Defender"),
	Attacker UMETA(DisplayName="Attacker"),
	Supporter UMETA(DisplayName="Supporter"),
};

UENUM()
enum class ECombatAIPreset : uint8
{
	Basic UMETA(DisplayName="Basic"),
	Aggressive UMETA(DisplayName="Aggressive"),
	Defensive UMETA(DisplayName="Defensive"),
};

UENUM()
enum class EEnemyCombatAIState : uint8
{
	Idle,
	Engage,
	Combat_Normal,
	Groggy_Stunned,
	Rising,
	Suppressed,// 체인/연출로 적 행동 금지
	ReturnToIdle,
};

USTRUCT()
struct FCombatAIWeights
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)float Attack = 1.0f;
	UPROPERTY(EditAnywhere)float Break = 1.0f;
	UPROPERTY(EditAnywhere)float TauntThreat = 1.0f;
	UPROPERTY(EditAnywhere)float Protect = 1.0f;
	UPROPERTY(EditAnywhere)float Heal = 1.0f;
	UPROPERTY(EditAnywhere)float Cleanse = 1.0f;
	UPROPERTY(EditAnywhere)float Buff = 1.0f;
	UPROPERTY(EditAnywhere)float Debuff = 1.0f;

	FCombatAIWeights operator+(const FCombatAIWeights &Rhs) const
	{
		FCombatAIWeights Out = *this;
		Out.Attack += Rhs.Attack;
		Out.Break += Rhs.Break;
		Out.TauntThreat += Rhs.TauntThreat;
		Out.Protect += Rhs.Protect;
		Out.Heal += Rhs.Heal;
		Out.Cleanse += Rhs.Cleanse;
		Out.Buff += Rhs.Buff;
		Out.Debuff += Rhs.Debuff;
		return Out;
	}
};

USTRUCT()
struct FCombatAISkillBucket
{
	GENERATED_BODY()

// 스킬 ID는 네 Skill 시스템에서 쓰는 키(FName/RowName 등)로 통일
	UPROPERTY(EditAnywhere) TArray<FName> TauntThreatSkills;
	UPROPERTY(EditAnywhere) TArray<FName> ProtectSkills;
	UPROPERTY(EditAnywhere) TArray<FName> BreakSkills;
	UPROPERTY(EditAnywhere) TArray<FName> DpsSkills;

	UPROPERTY(EditAnywhere) TArray<FName> HealSkills;
	UPROPERTY(EditAnywhere) TArray<FName> CleanseSkills;
	UPROPERTY(EditAnywhere) TArray<FName> BuffSkills;
	UPROPERTY(EditAnywhere) TArray<FName> DebuffSkills;
};

USTRUCT()
struct FCombatAIPartyTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FCombatAIWeights BaseWeights;
	UPROPERTY(EditAnywhere) FCombatAIWeights AggressiveOffset;
	UPROPERTY(EditAnywhere) FCombatAIWeights DefensiveOffset;

	// 서포터/수비 프리셋에서 특히 중요: 힐 트리거(HP%)
	UPROPERTY(EditAnywhere) float HealThreshold_Basic = 0.30f;
	UPROPERTY(EditAnywhere) float HealThreshold_Aggressive = 0.20f;
	UPROPERTY(EditAnywhere) float HealThreshold_Defensive = 0.45f;

	// 프리셋에 따른 “예약 스킬 AP 확보” 성향
	UPROPERTY(EditAnywhere) float ReservedApHoldStrength_Basic = 0.50f;
	UPROPERTY(EditAnywhere) float ReservedApHoldStrength_Aggressive = 0.20f;
	UPROPERTY(EditAnywhere) float ReservedApHoldStrength_Defensive = 0.85f;

	UPROPERTY(EditAnywhere) FCombatAISkillBucket Skills;
};

USTRUCT()
struct FEnemySkillPatternEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FName SkillId = NAME_None;

	// 가중치 랜덤
	UPROPERTY(EditAnywhere) float Weight = 1.0f;

	// 내부 쿨(적이 AP/쿨이 없을 때 문서 규약)
	UPROPERTY(EditAnywhere) float InternalCooldownRealSec = 3.0f;

	// 선택 조건(태그 기반 확장)
	UPROPERTY(EditAnywhere) FGameplayTagQuery RequireCasterTagQuery;
	UPROPERTY(EditAnywhere) FGameplayTagQuery RequireTargetTagQuery;

	// (옵션) 오프닝/브레이크 대응 등
	UPROPERTY(EditAnywhere) bool bOpeningOnly = false;
	UPROPERTY(EditAnywhere) int32 MaxOpeningUses = 1;
};

USTRUCT()
struct FEnemyAITuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)float DecisionIntervalRealSec = 0.20f;

	// Engage 시 초기 타겟 고정용 Threat
	UPROPERTY(EditAnywhere)float InitialThreatOnEngage = 50.0f;

	// ThreatComponent 쪽 Switch 규칙 값은 Threat 문서/테이블이 권위지만,
	// 여기 값은 “기본값 주입” 또는 “디버그용”으로만 사용 가능하게 둠.
	UPROPERTY(EditAnywhere)float TargetSwitchThresholdRatio = 1.15f;
	UPROPERTY(EditAnywhere)float TargetHoldSec = 1.20f;
	UPROPERTY(EditAnywhere)float SwitchCooldownSec = 0.25f;

	UPROPERTY(EditAnywhere)bool bRisingAttackAllowed = false;

	UPROPERTY(EditAnywhere)TArray<FEnemySkillPatternEntry> Pattern;
};
