#include "Game/JRPGPlayerPawn.h"

#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Movement/JRPGCharacterMovementComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Session/CombatZoneTrackerComponent.h"

AJRPGPlayerPawn::AJRPGPlayerPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJRPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	Locomotion = CreateDefaultSubobject<ULocomotionComponent>(TEXT("Locomotion"));
	ZoneTracker = CreateDefaultSubobject<UCombatZoneTrackerComponent>(TEXT("CombatZoneTracker"));
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	}
}

FVector AJRPGPlayerPawn::GetCameraTargetLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
}

FRotator AJRPGPlayerPawn::GetCameraTargetRotation() const
{
	if (AController* C = GetController())
		return C->GetControlRotation();
	
	return GetActorRotation();
}

float AJRPGPlayerPawn::GetCameraTargetArmLength() const
{
	return 550.0f; // 필드용 기본 값, 나중에 변경할 수 있음
}

void AJRPGPlayerPawn::UpdateCharacter(FName NewCharId)
{
	CurrentCharacterId = NewCharId;
	
	// 이후 AnimInstance(ABP)나 메쉬 같은거 변경여기서.-> 캐릭터 변경 변경시 여기서 호출하면 될 듯
}

ACombatCharacterActor* AJRPGPlayerPawn::GetCombatCharData() const
{
	// 전투 중에만 유효. 필드 상태에서는 SpawnedActorMap에 없으므로 nullptr 반환.

	if (UWorld* World = GetWorld())
		if (UPartyActorSpawnSubsystem* SpawnSub = World->GetSubsystem<UPartyActorSpawnSubsystem>())
			return SpawnSub->FindActorByCharacterID(CurrentCharacterId);
	return nullptr;
}
