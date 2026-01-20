#include "Skill_ThreatDown.h"
#include "JRPG/Combat/CombatTags.h"
#include "JRPG/Combat/Battle/BattleSessionSubsystem.h"
#include "JRPG/Combat/Threat/ThreatComponent.h"
#include "Engine/World.h"

USkill_ThreatDown::USkill_ThreatDown()
{
	SkillId = "ThreatDown";
	APCost = 1;
	CooldownSec = 10.f;
	Tags.AddTag(CombatTags::Skill_ThreatDown);
	bIgnoreRange = true;
}

void USkill_ThreatDown::Execute(AActor* User, AActor* Target)
{
	if (!User || !User->GetWorld()) return;

	if (UBattleSessionSubsystem* Session = User->GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		const TArray<AActor*> Enemies = Session->GetEnemiesRaw();
		for (AActor* Enemy : Enemies)
		{
			if (UThreatComponent* Threat = Enemy ? Enemy->FindComponentByClass<UThreatComponent>() : nullptr)
				Threat->ReduceThreat(User, ReduceAmount);
		}
	}
}