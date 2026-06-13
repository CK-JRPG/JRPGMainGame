#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Chain/ChainTypes.h"
#include "ChainSettingsDataAsset.generated.h"

USTRUCT()
struct FChainSettings
{
	GENERATED_BODY()

	// Selection(Freeze) window
	UPROPERTY(EditDefaultsOnly) float SelectionMaxRealSec = 2.5f;

	// 전체 체인 타임아웃(강제 Abort 안전장치)
	UPROPERTY(EditDefaultsOnly) float ChainMaxRealSec = 8.0f;

	// Time policy (Freeze)
	UPROPERTY(EditDefaultsOnly) float FrozenTimeScale = 0.01f;
	UPROPERTY(EditDefaultsOnly) float BlendInSec = 0.02f;
	UPROPERTY(EditDefaultsOnly) float BlendOutSec = 0.08f;

	// Step rules
	UPROPERTY(EditDefaultsOnly) EChainEligibilityPolicy EligibilityPolicy = EChainEligibilityPolicy::ChainOnlyEligible;
	UPROPERTY(EditDefaultsOnly) bool bAutoFillMissingSteps = true;
	UPROPERTY(EditDefaultsOnly) bool bAbortOnStepFail = true;

	// Enemy suppression
	UPROPERTY(EditDefaultsOnly) bool bSuppressEnemyAttacksDuringChain = true;
	UPROPERTY(EditDefaultsOnly) EEnemySuppressionScope EnemySuppressionScope = EEnemySuppressionScope::StopAndGate;

	// Optional: target lock (future 확장 훅)
	UPROPERTY(EditDefaultsOnly) bool bTargetLockDuringChain = true;

	// Finisher
	UPROPERTY(EditDefaultsOnly) FName FinisherSkillId = "Skill.ChainFinisher";
	UPROPERTY(EditDefaultsOnly) float FinisherDamageScalarPerTP = 0.02f;// TP 1당 +2%
	UPROPERTY(EditDefaultsOnly) float FinisherDamageScalarMax = 1.8f;// 최대 180%

	// Chain-Inside Groggy bonus (체인 중 새 스턴 발생 이후 피해 보너스)
	UPROPERTY(EditDefaultsOnly) float ChainStunBonusDamageMultiplier = 1.25f;

	// 전술 모드와 충돌 방지(체인 시작 시 전술 강제 종료)
	UPROPERTY(EditDefaultsOnly) bool bExitTacticalOnStart = true;

	// Status marker id (체인 보너스 자격 표식)
	UPROPERTY(EditDefaultsOnly) FName ChainStunVulnerableStatusId = "Debuff.ChainStunVulnerable";
};

UCLASS()
class JRPGCOMBAT_API UChainSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly) FChainSettings Settings;
};