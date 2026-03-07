#pragma once

#include "CoreMinimal.h"
#include "BasicCombatTypes.generated.h"

USTRUCT()
struct FDamageBreakdown
{
	GENERATED_BODY()

	UPROPERTY() float Attack = 0.f;
	UPROPERTY() float Defense = 0.f;

	UPROPERTY() float BasePower = 0.f;
	UPROPERTY() float AttackScale = 1.f;
	UPROPERTY() float DefenseScale = 0.5f;

	UPROPERTY() float PowerMultiplier = 1.f;

	UPROPERTY() float RawBeforeVariance = 0.f;
	UPROPERTY() float VarianceMultiplier = 1.f;

	UPROPERTY() bool bCritical = false;
	UPROPERTY() float CritChance = 0.f;
	UPROPERTY() float CritMultiplier = 1.5f;

	UPROPERTY() float FinalDamage = 0.f;

	UPROPERTY() float GroggyDamage = 0.f;
	UPROPERTY() float ThreatGenerated = 0.f;
};

USTRUCT()
struct FBasicAttackRequest
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Attacker;
	UPROPERTY() TWeakObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere) float BasePower = 5.f;
	UPROPERTY(EditAnywhere) float AttackScale = 1.0f;
	UPROPERTY(EditAnywhere) float DefenseScale = 0.5f;
	UPROPERTY(EditAnywhere) float PowerMultiplier = 1.0f;

	UPROPERTY(EditAnywhere) bool bAllowCrit = true;
	UPROPERTY(EditAnywhere) float VarianceMin = 0.95f;
	UPROPERTY(EditAnywhere) float VarianceMax = 1.05f;

	UPROPERTY(EditAnywhere) int32 APCost = 0;
	UPROPERTY(EditAnywhere) int32 SPGainOnHit = 0;
	UPROPERTY(EditAnywhere) int32 SPGainOnKill = 0;

	UPROPERTY(EditAnywhere) float GroggyPower = 8.f;
	UPROPERTY(EditAnywhere) float ThreatMultiplier = 1.0f;

	UPROPERTY(EditAnywhere) FName ReasonTag = "Combat.BasicAttack";
};

USTRUCT()
struct FCombatActionResult
{
	GENERATED_BODY()

	UPROPERTY() bool bOk = false;
	UPROPERTY() FName ReasonTag = NAME_None;

	UPROPERTY() TWeakObjectPtr<AActor> Attacker;
	UPROPERTY() TWeakObjectPtr<AActor> Target;

	UPROPERTY() FDamageBreakdown Breakdown;
	UPROPERTY() bool bTargetDied = false;

	static FCombatActionResult Ok()
	{
		FCombatActionResult R;
		R.bOk = true;
		return R;
	}

	static FCombatActionResult Fail(FName Reason)
	{
		FCombatActionResult R;
		R.bOk = false;
		R.ReasonTag = Reason;
		return R;
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBasicAttackResolved, const FCombatActionResult& /*Result*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatantDefeated, AActor* /*Victim*/, AActor* /*Killer*/);
