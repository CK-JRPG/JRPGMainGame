#include "Game/Companion/JRPGCompanionPawn.h"

#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Movement/JRPGCharacterMovementComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Game/Companion/CompanionPawnController.h"
//#include "Combat/Session/CombatZoneTrackerComponent.h"

AJRPGCompanionPawn::AJRPGCompanionPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJRPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	Locomotion = CreateDefaultSubobject<ULocomotionComponent>(TEXT("Locomotion"));
	//ZoneTracker = CreateDefaultSubobject<UCombatZoneTrackerComponent>(TEXT("CombatZoneTracker"));
	
	AIControllerClass = ACompanionPawnController::StaticClass();
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
	if (UWorld* World = GetWorld())
		if (UPartyActorSpawnSubsystem* SpawnSub = World->GetSubsystem<UPartyActorSpawnSubsystem>())
			return SpawnSub->FindActorByCharacterID(CurrentCharacterId);
	return nullptr;
}