#include "Game/Animation/FieldCharacterAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/Movement/LocomotionComponent.h"

void UFieldCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (!OwningCharacter)
	{
		return;
	}

	MovementComp = OwningCharacter->GetCharacterMovement();
	LocomotionComp = OwningCharacter->FindComponentByClass<ULocomotionComponent>();
}

void UFieldCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter || !MovementComp)
	{
		return;
	}

	const FVector Velocity = MovementComp->Velocity;
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	MovementDirection = CalculateDirection(Velocity, OwningCharacter->GetActorRotation());
	bIsInAir = MovementComp->IsFalling();

	const bool bHasAcceleration = MovementComp->GetCurrentAcceleration().SizeSquared() > 0.f;
	bShouldMove = GroundSpeed > MoveSpeedThreshold && bHasAcceleration;

	if (LocomotionComp)
	{
		bIsSprinting = LocomotionComp->IsSprinting();
		MoveInput = LocomotionComp->GetMoveInput();
	}
	else
	{
		bIsSprinting = false;
		MoveInput = FVector2D::ZeroVector;
	}
}
