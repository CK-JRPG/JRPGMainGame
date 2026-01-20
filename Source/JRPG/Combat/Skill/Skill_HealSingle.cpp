#include "Skill_HealSingle.h"
#include "JRPG/Combat/CombatTags.h"
#include "JRPG/Combat/Stats/HealthComponent.h"
#include "JRPG/Combat/Battle/BattleSessionSubsystem.h"
#include "JRPG/Combat/Threat/ThreatComponent.h"
#include "Engine/World.h"

USkill_HealSingle::USkill_HealSingle()
{
	SkillId = "HealSingle";
	APCost = 1;
	CooldownSec = 6.f;
	Tags.AddTag(CombatTags::Skill_HealSingle);
	bIgnoreRange = true;
}

void USkill_HealSingle::Execute(AActor* User, AActor* Target)
{
	if (!User || !Target || !User->GetWorld()) return;

	if (UHealthComponent* HP = Target->FindComponentByClass<UHealthComponent>())
		HP->ApplyHeal(HealAmount);

	if (UBattleSessionSubsystem* Session = User->GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		const TArray<AActor*> Enemies = Session->GetEnemiesRaw();
		const float ThreatGain = HealAmount * HealThreatMultiplier;

		for (AActor* Enemy : Enemies)
		{
			if (UThreatComponent* Threat = Enemy ? Enemy->FindComponentByClass<UThreatComponent>() : nullptr)
				Threat->AddThreat(User, ThreatGain);
		}
	}
}