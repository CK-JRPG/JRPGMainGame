#include "Skill_BasicAttack.h"
#include "JRPG/Combat/CombatTags.h"
#include "JRPG/Combat/Stats/HealthComponent.h"
#include "JRPG/Combat/Skill/SkillComponent.h"
#include "JRPG/Combat/Threat/ThreatComponent.h"
#include "JRPG/Combat/Encounter/EncounterSubsystem.h"
#include "Perception/AISense_Damage.h"

USkill_BasicAttack::USkill_BasicAttack()
{
	SkillId = "BasicAttack";
	APCost = 0;
	CooldownSec = 0.f;
	Tags.AddTag(CombatTags::Skill_DpsLow);
	AttackType = EAttackType::TargetAttack;
	bIgnoreRange = true;
}

void USkill_BasicAttack::Execute(AActor* User, AActor* Target)
{
	if (!User || !Target || !User->GetWorld()) return;

	// 피격 인카운터
	if (UEncounterSubsystem* Enc = User->GetWorld()->GetSubsystem<UEncounterSubsystem>())
		Enc->RequestEncounter(Target, User, EEncounterTrigger::Hit);

	if (UHealthComponent* HP = Target->FindComponentByClass<UHealthComponent>())
		HP->ApplyDamage(Damage);

	// Damage Sense(시야 인카운터가 아니어도 “맞으면 전투”가 확실히 들어오게)
	UAISense_Damage::ReportDamageEvent(User->GetWorld(), Target, User, Damage, User->GetActorLocation(), Target->GetActorLocation());

	if (USkillComponent* SkillComp = User->FindComponentByClass<USkillComponent>())
		SkillComp->ReduceAllCooldownsByPercent(CooldownReducePercentOnHit);

	if (UThreatComponent* Threat = Target->FindComponentByClass<UThreatComponent>())
		Threat->AddThreat(User, Damage * ThreatOnHitMultiplier);
}