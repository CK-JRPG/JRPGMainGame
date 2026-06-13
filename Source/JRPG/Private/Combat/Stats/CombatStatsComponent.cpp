// Source/JRPGCombat/Private/Combat/Stats/CombatStatsComponent.cpp
#include "Combat/Stats/CombatStatsComponent.h"

float UCombatStatsComponent::ComputeFinal(float BaseValue, const FStatModifierAccumulator& A)
{
	const float FlatApplied = BaseValue + A.Flat;
	return FlatApplied* (1.0f + A.Pct);
}

FFinalStatsSnapshot UCombatStatsComponent::ToSnapshot(const FCombatFinalStats& InFinal)
{
	FFinalStatsSnapshot Out;

	Out.Attack = InFinal.Attack;
	Out.Defense = InFinal.Defense;
	Out.MaxHP = InFinal.MaxHP;
	Out.BreakPower = InFinal.BreakPower;
	Out.HealingPower = InFinal.HealingPower;
	Out.ThreatMod = InFinal.ThreatMod;
	
	return Out;
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

	CachedBreakdown = FStatsBreakdownSnapshot();
	CachedBreakdown.Base.Attack = Base.Attack;
	CachedBreakdown.Base.Defense = Base.Defense;
	CachedBreakdown.Base.MaxHP = Base.MaxHP;
	CachedBreakdown.Base.BreakPower = Base.BreakPower;
	CachedBreakdown.Base.HealingPower = Base.HealingPower;
	CachedBreakdown.Base.ThreatMod = Base.ThreatMod;
	
	CachedBreakdown.Equipment.Attack = Final.Attack - Base.Attack;
	CachedBreakdown.Equipment.Defense = Final.Defense - Base.Defense;
	CachedBreakdown.Equipment.MaxHP = Final.MaxHP - Base.MaxHP;
	CachedBreakdown.Equipment.BreakPower = Final.BreakPower - Base.BreakPower;
	CachedBreakdown.Equipment.HealingPower = Final.HealingPower - Base.HealingPower;
	CachedBreakdown.Equipment.ThreatMod = Final.ThreatMod - Base.ThreatMod;
	CachedBreakdown.Final = ToSnapshot(Final);

	OnFinalStatsChanged.Broadcast(Final);
	OnStatsSnapshotChanged.Broadcast(CharacterId);
}

FStatsPreviewDelta UCombatStatsComponent::BuildPreviewDelta(const FAugmentModifierSet& CandidateMods) const
{
	FStatsPreviewDelta Delta;

	const float PreviewAttack = ComputeFinal(Base.Attack, CandidateMods.Attack);
	const float PreviewDefense = ComputeFinal(Base.Defense, CandidateMods.Defense);
	const float PreviewHp = ComputeFinal(Base.MaxHP, CandidateMods.HP);
	const float PreviewBreakPower = ComputeFinal(Base.BreakPower, CandidateMods.BreakPower);
	const float PreviewHealingPower = ComputeFinal(Base.HealingPower, CandidateMods.HealingPower);
	const float PreviewThreatMod = Base.ThreatMod * (1.0f + CandidateMods.ThreatModPct);

	Delta.Delta.Attack = PreviewAttack - Final.Attack;
	Delta.Delta.Defense = PreviewDefense - Final.Defense;
	Delta.Delta.MaxHP = PreviewHp - Final.MaxHP;
	Delta.Delta.BreakPower = PreviewBreakPower - Final.BreakPower;
	Delta.Delta.HealingPower = PreviewHealingPower - Final.HealingPower;
	Delta.Delta.ThreatMod = PreviewThreatMod - Final.ThreatMod;
	
	return Delta;
}
