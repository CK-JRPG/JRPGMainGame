#include "Combat/Battle/CombatFormulaLibrary.h"

FDamageBreakdown UCombatFormulaLibrary::BuildDamage(
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
float ThreatMultiplier)
{
	FDamageBreakdown B;
	B.Attack = AttackerAttack;
	B.Defense = TargetDefense;
	B.BasePower = BasePower;
	B.AttackScale = AttackScale;
	B.DefenseScale = DefenseScale;
	B.PowerMultiplier = PowerMultiplier;

	B.CritChance = FMath::Clamp(CritChance, 0.f, 1.f);
	B.CritMultiplier = 1.5f + FMath::Max(0.f, CritBonusDamage);

	const float Raw = BasePower + AttackerAttack * AttackScale - TargetDefense * DefenseScale;
	B.RawBeforeVariance = FMath::Max(1.f, Raw) * FMath::Max(0.f, PowerMultiplier);

	const float VMin = FMath::Min(VarianceMin, VarianceMax);
	const float VMax = FMath::Max(VarianceMin, VarianceMax);
	B.VarianceMultiplier = FMath::FRandRange(VMin, VMax);

	float Damage = B.RawBeforeVariance * B.VarianceMultiplier;

	if (bAllowCrit && FMath::FRand() <= B.CritChance)
	{
		B.bCritical = true;
		Damage *= B.CritMultiplier;
	}

	B.FinalDamage = FMath::Max(1.f, FMath::FloorToFloat(Damage));
	B.GroggyDamage = FMath::Max(0.f, GroggyPower);
	B.ThreatGenerated = FMath::Max(0.f, B.FinalDamage * FMath::Max(0.f, ThreatMultiplier));

	return B;
}
