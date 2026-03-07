#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "CombatFormulaLibrary.generated.h"

UCLASS()
class JRPG_API UCombatFormulaLibrary :public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FDamageBreakdown BuildDamage(
	float AttackerAttack,
	float TargetDefense,
	float BasePower,
	float AttackScale,
	float DefenseScale,
	float PowerMultiplier,
	bool bAllowCrit,
	float CritChance,
	float CritBonusDamage,
	float VarianceMin,
	float VarianceMax,
	float GroggyPower,
	float ThreatMultiplier);
};
