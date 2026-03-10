#include "Combat/Characters/Stats/CharacterCombatStatsComponent.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

#if __has_include("Combat/Progression/Leveling/LevelingSubsystem.h")
#include "Combat/Progression/Leveling/LevelingSubsystem.h"
#define JRPG_HAS_LEVELING 1
#else
#define JRPG_HAS_LEVELING 0
#endif

UCharacterCombatStatsComponent::UCharacterCombatStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCharacterCombatStatsComponent::BeginPlay()
{
	Super::BeginPlay();
	CharacterComp = GetOwner() ? GetOwner()->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
	HP = GetOwner() ? GetOwner()->FindComponentByClass<UHPComponent>() : nullptr;
	AP = GetOwner() ? GetOwner()->FindComponentByClass<UAPComponent>() : nullptr;
	SP = GetOwner() ? GetOwner()->FindComponentByClass<USPComponent>() : nullptr;
	RecomputeStats("Init");
}

int32 UCharacterCombatStatsComponent::QueryPartyLevel() const
{
#if JRPG_HAS_LEVELING
	if (GetWorld() && GetWorld()->GetGameInstance())
		if (ULevelingSubsystem* L = GetWorld()->GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
			return L->GetPartyLevel();
#endif
	return 1;
}

void UCharacterCombatStatsComponent::AddModifier(const FCombatStatModifier& Mod)
{
	Mods.Add(Mod); 
	RecomputeStats("Mod.Add");
}

void UCharacterCombatStatsComponent::RemoveModifiersBySource(UObject* Source)
{
	if (!Source) return;
	Mods.RemoveAll([Source](const FCombatStatModifier& M){ return M.Source.Get() == Source; });
	RecomputeStats("Mod.RemoveBySource");
}

void UCharacterCombatStatsComponent::ClearModifiers()
{
	Mods.Reset(); 
	RecomputeStats("Mod.Clear");
}

float UCharacterCombatStatsComponent::GetStatFloat(ECombatStat Stat) const
{
	switch (Stat)
	{
		case ECombatStat::Attack:		return Snapshot.Attack;
		case ECombatStat::Defense:		return Snapshot.Defense;
		case ECombatStat::Speed:		return Snapshot.Speed;
		case ECombatStat::MaxHP:		return Snapshot.MaxHP;
		case ECombatStat::MaxAP:		return (float)Snapshot.MaxAP;
		case ECombatStat::MaxSP:		return (float)Snapshot.MaxSP;
		case ECombatStat::CritRate:		return Snapshot.CritRate;
		case ECombatStat::CritDamage:	return Snapshot.CritDamage;
		case ECombatStat::BreakPower:	return Snapshot.BreakPower;
		case ECombatStat::HealingPower: return Snapshot.HealingPower;
		default: 
		
		return 0.f;
	}
}

void UCharacterCombatStatsComponent::ApplyMods(ECombatStat Stat,float&InOutValue) const
{
	for (const FCombatStatModifier& M : Mods)
		if (M.Stat == Stat && M.Op == EStatModOp::Add) InOutValue += M.Value;

	float Mul = 1.f;
	for (const FCombatStatModifier& M : Mods)
		if (M.Stat == Stat && M.Op == EStatModOp::Mul) Mul *= M.Value;

	InOutValue *= Mul;
}

void UCharacterCombatStatsComponent::ApplyToResources(bool bKeepHPRatio)
{
	if (HP.IsValid()) HP->SetMaxHP(Snapshot.MaxHP, bKeepHPRatio);
	if (AP.IsValid()) AP->InitializeAP(Snapshot.MaxAP, true);
	if (SP.IsValid()) SP->InitializeSP(Snapshot.MaxSP, SP->GetSP());
}

void UCharacterCombatStatsComponent::RecomputeStats(FName)
{
	const int32 Level = QueryPartyLevel();

	float BaseAtk = 10.f, BaseDef = 5.f, BaseSpd = 10.f;
	float BaseMaxHP = 100.f; int32 BaseMaxAP = 10; int32 BaseMaxSP = 100;

	if (CharacterComp.IsValid() && CharacterComp->CharacterDef)
	{
		const auto& P = CharacterComp->CharacterDef->BaseParams;
		BaseAtk = P.BaseAttack; BaseDef = P.BaseDefense; BaseSpd = P.BaseSpeed;
		BaseMaxHP = P.MaxHP; BaseMaxAP = P.MaxAP; BaseMaxSP = P.MaxSP;
	}

	float Atk = BaseAtk * LevelScaling.MulByLevel(LevelScaling.AttackPerLevelMul, Level);
	float Def = BaseDef * LevelScaling.MulByLevel(LevelScaling.DefensePerLevelMul, Level);
	float Spd = BaseSpd * LevelScaling.MulByLevel(LevelScaling.SpeedPerLevelMul, Level);

	float MaxHP = BaseMaxHP * LevelScaling.MulByLevel(LevelScaling.HPPerLevelMul, Level);
	float MaxAPf = (float)BaseMaxAP;
	float MaxSPf = (float)BaseMaxSP;

	float CritRate = 0.f, CritDmg = 0.f, BreakPower = 0.f, HealingPower = 0.f;

	ApplyMods(ECombatStat::Attack, Atk);
	ApplyMods(ECombatStat::Defense, Def);
	ApplyMods(ECombatStat::Speed, Spd);
	ApplyMods(ECombatStat::MaxHP, MaxHP);
	ApplyMods(ECombatStat::MaxAP, MaxAPf);
	ApplyMods(ECombatStat::MaxSP, MaxSPf);
	ApplyMods(ECombatStat::CritRate, CritRate);
	ApplyMods(ECombatStat::CritDamage, CritDmg);
	ApplyMods(ECombatStat::BreakPower, BreakPower);
	ApplyMods(ECombatStat::HealingPower, HealingPower);

	MaxHP = FMath::Max(1.f, MaxHP);
	const int32 MaxAP = FMath::Max(0, (int32)FMath::RoundToInt(MaxAPf));
	const int32 MaxSP = FMath::Max(0, (int32)FMath::RoundToInt(MaxSPf));

	Snapshot.Attack = Atk; Snapshot.Defense = Def; Snapshot.Speed = Spd;
	Snapshot.MaxHP = MaxHP; Snapshot.MaxAP = MaxAP; Snapshot.MaxSP = MaxSP;
	Snapshot.CritRate = CritRate; Snapshot.CritDamage = CritDmg;
	Snapshot.BreakPower = BreakPower; Snapshot.HealingPower = HealingPower;

	ApplyToResources(true);
	OnCombatStatsRecomputed.Broadcast(Snapshot);
}