#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "Combat/SP/SynergyPointTypes.h"
#include "SynergyPointSettingsDataAsset.generated.h"

/**
 * 문서 8.2 테이블/DA 컬럼 정의(SSOT)
 * - SPCap
 * - SPMaxGainPerSec
 * - SameEventCooldownSec
 * - TacticalRoleBonusMultiplier
 * - OvercapAllowed
 * + BaseGain 값/RoleBonus 파라미터(문서에 “테이블만 정의, 값 TBD”)
 */
USTRUCT()
struct FSynergyPointSettings
{
	GENERATED_BODY()

	// Core
	UPROPERTY(EditDefaultsOnly) int32 SPCap = 100;
	UPROPERTY(EditDefaultsOnly) int32 SPMaxGainPerSec = 60;
	UPROPERTY(EditDefaultsOnly) float SameEventCooldownSec = 3.0f;

	UPROPERTY(EditDefaultsOnly) float TacticalRoleBonusMultiplier = 1.3f; // 문서 5.3.3
	UPROPERTY(EditDefaultsOnly) int32 TacticalFlatBonus = 0;             // 문서에 존재(옵션)
	UPROPERTY(EditDefaultsOnly) bool bOvercapAllowed = true;

	// Base gain (문서 5.1) : “의미 있는 결과 최소 보상”
	UPROPERTY(EditDefaultsOnly) int32 BaseGain_Damage = 2;
	UPROPERTY(EditDefaultsOnly) int32 BaseGain_Heal = 2;
	UPROPERTY(EditDefaultsOnly) int32 BaseGain_Debuff = 2;
	UPROPERTY(EditDefaultsOnly) int32 BaseGain_Buff = 1;
	UPROPERTY(EditDefaultsOnly) int32 BaseGain_Cleanse = 3;
	UPROPERTY(EditDefaultsOnly) int32 BaseGain_Taunt = 2;
	UPROPERTY(EditDefaultsOnly) int32 BaseGain_Break = 2;

	// Defender role bonus (문서 5.2.1)
	UPROPERTY(EditDefaultsOnly) float DefenderAggroHoldWindowSec = 3.0f;
	UPROPERTY(EditDefaultsOnly) int32 DefenderAggroHoldBonus = 10;

	UPROPERTY(EditDefaultsOnly) int32 DefenderAggroRescueBonus = 18;
	UPROPERTY(EditDefaultsOnly) int32 DefenderPartyProtectBonus = 12;

	// Attacker role bonus (문서 5.2.2)
	UPROPERTY(EditDefaultsOnly) float AttackerBreakContributionCoeff = 0.25f; // breakAmount * coeff
	UPROPERTY(EditDefaultsOnly) int32 AttackerBreakContributionCap = 20;

	UPROPERTY(EditDefaultsOnly) int32 AttackerStunTriggerBonus = 20;

	UPROPERTY(EditDefaultsOnly) float AttackerDamageWindowSec = 3.0f;
	UPROPERTY(EditDefaultsOnly) float AttackerDamageWindowThreshold = 200.f;
	UPROPERTY(EditDefaultsOnly) int32 AttackerDamageWindowBonus = 15;

	// Supporter role bonus (문서 5.2.3)
	UPROPERTY(EditDefaultsOnly) float SupporterCriticalHealHPThreshold = 0.30f; // hp<30%
	UPROPERTY(EditDefaultsOnly) int32 SupporterCriticalHealBonus = 18;

	UPROPERTY(EditDefaultsOnly) int32 SupporterCleanseBonus = 15;

	// Buff uptime은 “tick형 이벤트”로 넣을 수 있게만 제공
	UPROPERTY(EditDefaultsOnly) float SupporterBuffUptimeSecStep = 3.0f;
	UPROPERTY(EditDefaultsOnly) int32 SupporterBuffUptimeBonus = 10;
};

UCLASS()
class JRPGCOMBAT_API USynergyPointSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly) FSynergyPointSettings Settings;
};