#include "Game/JRPGPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
#include "Combat/Movement/JRPGCharacterMovementComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Session/CombatZoneTrackerComponent.h"
#include "GameFramework/SpringArmComponent.h"

AJRPGPlayerPawn::AJRPGPlayerPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJRPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	Locomotion = CreateDefaultSubobject<ULocomotionComponent>(TEXT("Locomotion"));
	ZoneTracker = CreateDefaultSubobject<UCombatZoneTrackerComponent>(TEXT("CombatZoneTracker"));
	
	// 카메라 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	}
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
}

void AJRPGPlayerPawn::UpdateCharacter(FName NewCharId)
{
	CurrentCharacterId = NewCharId;
	
	// 이후 AnimInstance(ABP)나 메쉬 같은거 변경여기서.-> 캐릭터 변경 변경시 여기서 호출하면 될 듯
}

ACombatCharacterActor* AJRPGPlayerPawn::GetCombatCharData() const
{
	UCombatCharacterRegistrySubsystem* RegistrySubsystem = GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>();
	if (!RegistrySubsystem) 	
		return nullptr;
	
	return Cast<ACombatCharacterActor>(RegistrySubsystem->FindById(CurrentCharacterId));
}
