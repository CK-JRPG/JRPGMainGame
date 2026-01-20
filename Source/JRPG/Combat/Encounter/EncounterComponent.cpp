#include "EncounterComponent.h"

#include "JRPG/Combat/Encounter/EncounterSubsystem.h"
#include "Engine/World.h"

void UEncounterComponent::NotifyHitBy(AActor* InstigatorActor)
{
	if (!GetWorld() || !GetOwner() || !InstigatorActor) return;

	if (UEncounterSubsystem* Enc = GetWorld()->GetSubsystem<UEncounterSubsystem>())
		Enc->RequestEncounter(GetOwner(), InstigatorActor, EEncounterTrigger::Hit);
}