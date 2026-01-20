#include "TacticalModeSubsystem.h"
#include "JRPG/Combat/Time/CombatTimeSubsystem.h"
#include "JRPG/Combat/Skill/SkillComponent.h"
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "Engine/World.h"

UCombatTimeSubsystem* UTacticalModeSubsystem::GetTime() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatTimeSubsystem>() : nullptr;
}

void UTacticalModeSubsystem::EnterTactical(float DurationRealSec, float Dilation)
{
	if (UCombatTimeSubsystem* Time = GetTime())
	{
		Time->EnterTactical(DurationRealSec, Dilation);
		OnTacticalEntered.Broadcast();
	}
}

void UTacticalModeSubsystem::ExitTactical()
{
	if (UCombatTimeSubsystem* Time = GetTime())
	{
		Time->ExitTactical();
		OnTacticalExited.Broadcast();
	}
}

bool UTacticalModeSubsystem::IsTacticalActive() const
{
	if (const UCombatTimeSubsystem* Time = GetTime())
		return Time->IsTacticalActive();
	return false;
}

bool UTacticalModeSubsystem::ReserveSkillById(AActor* Character, FName SkillId)
{
	if (!Character) return false;

	if (USkillComponent* Skills = Character->FindComponentByClass<USkillComponent>())
	{
		if (UCombatSkill* Skill = Skills->FindSkillById(SkillId))
		{
			Skills->ReserveSkill(Skill);
			return true;
		}
	}
	return false;
}

void UTacticalModeSubsystem::ClearReservation(AActor* Character)
{
	if (!Character) return;
	if (USkillComponent* Skills = Character->FindComponentByClass<USkillComponent>())
		Skills->ClearReservedSkill();
}