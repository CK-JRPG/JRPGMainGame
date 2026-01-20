#include "Skill_Taunt.h"
#include "JRPG/Combat/CombatTags.h"
#include "JRPG/Combat/Threat/ThreatComponent.h"

USkill_Taunt::USkill_Taunt()
{
	SkillId = "Taunt";
	APCost = 1;
	CooldownSec = 8.f;
	Tags.AddTag(CombatTags::Skill_Taunt);
	bIgnoreRange = true;
}

void USkill_Taunt::Execute(AActor* User, AActor* Target)
{
	if (!User || !Target) return;

	if (UThreatComponent* Threat = Target->FindComponentByClass<UThreatComponent>())
	{
		Threat->AddThreat(User, ThreatBoost);
		Threat->ForceTarget(User, ForceDurationSec);
	}
}