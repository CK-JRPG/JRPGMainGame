// Source/JRPGCombat/Public/Combat/AI/CombatAIScorer.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/AI/CombatAITypes.h"

class UCombatAIContext;
class UCombatAIPresetAsset;

class JRPG_API FCombatAIScorer
{
public:
	// Party AI
	static FCombatAIAction ChoosePartyAction(
		const UCombatAIContext &Ctx,
		const UCombatAIPresetAsset &PresetAsset,
		ECombatPartyRole Role,
		ECombatAIPreset Preset);

	// Enemy AI
	static FCombatAIAction ChooseEnemyAction(
		const UCombatAIContext &Ctx,
		const UCombatAIPresetAsset &PresetAsset,
		EEnemyCombatAIState State,
		AActor *CurrentTarget);

private:
	// Helpers
	static bool CanUseAnySkill(const UCombatAIContext &Ctx, const TArray<FName> &SkillIds, FName &OutChosen);
	static AActor* FindMostCriticalAlly(const UCombatAIContext &Ctx, float HpThreshold);
	static bool AllyHasCC(const UCombatAIContext &Ctx, AActor *&OutAllyWithCC);

	static float ApplyReservationHoldPenalty(
		const UCombatAIContext &Ctx,
		const UCombatAIPresetAsset &PresetAsset,
		ECombatAIPreset Preset,
		float RawScore);
};