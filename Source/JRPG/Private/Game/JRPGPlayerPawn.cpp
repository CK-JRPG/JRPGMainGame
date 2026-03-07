#include "Game/JRPGPlayerPawn.h"

#include "Combat/Movement/JRPGCharacterMovementComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Session/CombatZoneTrackerComponent.h"

AJRPGPlayerPawn::AJRPGPlayerPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJRPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	Locomotion = CreateDefaultSubobject<ULocomotionComponent>(TEXT("Locomotion"));
	ZoneTracker = CreateDefaultSubobject<UCombatZoneTrackerComponent>(TEXT("CombatZoneTracker"));
}