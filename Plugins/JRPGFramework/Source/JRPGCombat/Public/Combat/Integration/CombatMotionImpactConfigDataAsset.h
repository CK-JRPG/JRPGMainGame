#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Movement/CombatMotionTypes.h"
#include "CombatMotionImpactConfigDataAsset.generated.h"

USTRUCT()
struct FCombatMotionImpactRule
{
	GENERATED_BODY()

	// Status
	UPROPERTY(EditDefaultsOnly) bool bApplyStatus = false;
	UPROPERTY(EditDefaultsOnly) FName StatusId = NAME_None;
	UPROPERTY(EditDefaultsOnly) float StatusDuration = 1.2f;

	// Groggy
	UPROPERTY(EditDefaultsOnly) bool bAddGroggy = false;
	UPROPERTY(EditDefaultsOnly) float GroggyBreakAmount = 25.f;

	// SP
	UPROPERTY(EditDefaultsOnly) bool bGrantSP = false;
	UPROPERTY(EditDefaultsOnly) int32 SPAmount = 10;

	// Threat
	UPROPERTY(EditDefaultsOnly) bool bLockThreatToInstigator = false;
	UPROPERTY(EditDefaultsOnly) float ThreatLockDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly) bool bAddThreatToInstigator = false;
	UPROPERTY(EditDefaultsOnly) float ThreatAmount = 20.f;
};

UCLASS()
class JRPGCOMBAT_API UCombatMotionImpactConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TMap<ECombatMotionArchetype, FCombatMotionImpactRule> Rules;

	bool TryGetRule(ECombatMotionArchetype Archetype, FCombatMotionImpactRule& Out) const
	{
		if (const FCombatMotionImpactRule* R = Rules.Find(Archetype))
		{
			Out = *R;
			return true;
		}
		return false;
	}
};
