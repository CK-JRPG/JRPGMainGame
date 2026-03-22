#include "Game/Companion/JRPGCompanionPawn.h"

#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
#include "Combat/Movement/JRPGCharacterMovementComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Session/CombatZoneTrackerComponent.h"

AJRPGCompanionPawn::AJRPGCompanionPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJRPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	Locomotion = CreateDefaultSubobject<ULocomotionComponent>(TEXT("Locomotion"));
	ZoneTracker = CreateDefaultSubobject<UCombatZoneTrackerComponent>(TEXT("CombatZoneTracker"));
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		// MoveComp->bUseRVOAvoidance = true; 
	}
}

void AJRPGCompanionPawn::UpdateCharacter(FName NewCharId)
{
	CurrentCharacterId = NewCharId;
	
	// TODO: AnimInstance나 메쉬, 스켈레톤 변경 로직을 여기에 구현
}

ACombatCharacterActor* AJRPGCompanionPawn::GetCombatCharData() const
{
	UCombatCharacterRegistrySubsystem* RegistrySubsystem = GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>();
	if (!RegistrySubsystem) 	
		return nullptr;
	
	return Cast<ACombatCharacterActor>(RegistrySubsystem->FindById(CurrentCharacterId));
}