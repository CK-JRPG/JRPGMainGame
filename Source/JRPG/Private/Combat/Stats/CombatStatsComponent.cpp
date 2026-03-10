// Source/JRPGCombat/Private/Combat/Stats/CombatStatsComponent.cpp
#include "Combat/Stats/CombatStatsComponent.h"

float UCombatStatsComponent::ComputeFinal(float BaseValue, const FStatModifierAccumulator& A)
{
	const float FlatApplied = BaseValue + A.Flat;
	return FlatApplied* (1.0f + A.Pct);
}

void UCombatStatsComponent::ApplyAugmentModifierSet(const FAugmentModifierSet& Mods)
{
	Final.Attack = ComputeFinal(Base.Attack, Mods.Attack);
	Final.Defense = ComputeFinal(Base.Defense, Mods.Defense);
	Final.MaxHP = ComputeFinal(Base.MaxHP, Mods.HP);

	Final.BreakPower = ComputeFinal(Base.BreakPower, Mods.BreakPower);
	Final.HealingPower = ComputeFinal(Base.HealingPower, Mods.HealingPower);

	// ThreatModPct는 “(1 + pct)” 개념
	Final.ThreatMod = Base.ThreatMod * (1.0f + Mods.ThreatModPct);

	OnFinalStatsChanged.Broadcast(Final);
}