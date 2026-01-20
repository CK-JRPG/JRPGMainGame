#include "Skill_Damage.h"
#include "JRPG/Combat/CombatTags.h"
#include "JRPG/Combat/Stats/HealthComponent.h"
#include "JRPG/Combat/Threat/ThreatComponent.h"
#include "JRPG/Combat/Encounter/EncounterSubsystem.h"
#include "Perception/AISense_Damage.h"

USkill_Damage::USkill_Damage()
{
	SkillId = "Damage";
	APCost = 1;
	CooldownSec = 4.f;
	Tags.AddTag(CombatTags::Skill_DpsHigh);
	bIgnoreRange = true;
}

void USkill_Damage::Execute(AActor* User, AActor* Target)
{
	if (!User || !Target || !User->GetWorld()) return;

	if (UEncounterSubsystem* Enc = User->GetWorld()->GetSubsystem<UEncounterSubsystem>())
		Enc->RequestEncounter(Target, User, EEncounterTrigger::Hit);

	if (UHealthComponent* HP = Target->FindComponentByClass<UHealthComponent>())
		HP->ApplyDamage(Damage);

	UAISense_Damage::ReportDamageEvent(User->GetWorld(), Target, User, Damage, User->GetActorLocation(), Target->GetActorLocation());

	if (UThreatComponent* Threat = Target->FindComponentByClass<UThreatComponent>())
		Threat->AddThreat(User, Damage);
}