#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Skills/SkillComponent.h"

UCombatActionComponent::UCombatActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatActionComponent::BeginPlay()
{
	Super::BeginPlay();
	CharacterComp = GetOwner() ? GetOwner()->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
	SkillComp = GetOwner() ? GetOwner()->FindComponentByClass<USkillComponent>() : nullptr;
}

UBasicCombatSubsystem* UCombatActionComponent::GetCombatSubsystem() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBasicCombatSubsystem>() : nullptr;
}

FCombatActionResult UCombatActionComponent::TryBasicAttack(AActor* Target)
{
	UBasicCombatSubsystem* CS = GetCombatSubsystem();
	if (!CS) return FCombatActionResult::Fail("Reject.NoCombatSubsystem");
	if (!CharacterComp.IsValid() || !CharacterComp->CharacterDef) return FCombatActionResult::Fail("Reject.NoCharacterDef");

	const UCombatCharacterDataAsset* Def = CharacterComp->CharacterDef;

	FBasicAttackRequest Req;
	Req.Attacker = GetOwner();
	Req.Target = Target;

	Req.BasePower = (BasicAttackBasePowerOverride > 0.f) ? BasicAttackBasePowerOverride : Def->BasicAttackBasePower;
	Req.AttackScale = (BasicAttackAttackScaleOverride>0.f) ? BasicAttackAttackScaleOverride : Def->BasicAttackAttackScale;
	Req.DefenseScale = (BasicAttackDefenseScaleOverride>0.f) ? BasicAttackDefenseScaleOverride : Def->BasicAttackDefenseScale;

	Req.APCost = (BasicAttackAPCostOverride >= 0) ? BasicAttackAPCostOverride : Def->BasicAttackAPCost;
	Req.SPGainOnHit = (BasicAttackSPGainOnHitOverride >= 0) ? BasicAttackSPGainOnHitOverride : Def->BasicAttackSPGainOnHit;
	Req.SPGainOnKill = (BasicAttackSPGainOnKillOverride >= 0) ? BasicAttackSPGainOnKillOverride : Def->BasicAttackSPGainOnKill;

	Req.GroggyPower = (BasicAttackGroggyPowerOverride >= 0.f) ? BasicAttackGroggyPowerOverride : Def->BasicAttackGroggyPower;
	Req.ThreatMultiplier = (BasicAttackThreatMultiplierOverride >= 0.f) ? BasicAttackThreatMultiplierOverride : Def->BasicAttackThreatMultiplier;

	Req.ReasonTag = "Combat.BasicAttack";
	return CS->ExecuteBasicAttack(Req);
}

FSkillCastResult UCombatActionComponent::TryCastSkill(FName SkillId, const TArray<AActor*>& Targets)
{
	if (!SkillComp.IsValid()) return FSkillCastResult::Fail("Reject.NoSkillComponent");
	return SkillComp->CastSkill(SkillId, Targets, "Combat.SkillCast");
}