// Source/JRPGCombat/Public/Combat/AI/CombatAIPresetAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/AI/CombatAITypes.h"
#include "CombatAIPresetAsset.generated.h"

UCLASS()
class JRPG_API UCombatAIPresetAsset :public UDataAsset
{
	GENERATED_BODY()

public:
	// Party
	UPROPERTY(EditAnywhere, Category = "Party")
	FCombatAIPartyTuning Defender;

	UPROPERTY(EditAnywhere, Category = "Party")
	FCombatAIPartyTuning Attacker;

	UPROPERTY(EditAnywhere, Category="Party")
	FCombatAIPartyTuning Supporter;

	// Party 의사결정 주기(RealTime)
	UPROPERTY(EditAnywhere, Category = "Party")
	float PartyDecisionIntervalRealSec = 0.15f;

	// Enemy
	UPROPERTY(EditAnywhere, Category = "Enemy")
	FEnemyAITuning Enemy;

public:
	const FCombatAIPartyTuning &GetPartyTuning(ECombatPartyRole Role) const;

	FCombatAIWeights GetEffectiveWeights(ECombatPartyRole Role,ECombatAIPreset Preset) const;

	float GetHealThreshold(ECombatAIPreset Preset) const;
	float GetReservedApHoldStrength(ECombatAIPreset Preset) const;
};