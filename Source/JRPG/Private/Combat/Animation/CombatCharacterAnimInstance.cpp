#include "Combat/Animation/CombatCharacterAnimInstance.h"

#include "Combat/Characters/CombatCharacterActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/Motion/CombatMotionComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Battle/CombatActionComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Stats/HPComponent.h"

void UCombatCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCombatActor = Cast<ACombatCharacterActor>(TryGetPawnOwner());
	if (!OwningCombatActor)
	{
		return;
	}

	MovementComp = OwningCombatActor->GetCharacterMovement();
	MotionComp = OwningCombatActor->MotionComp;
	GroggyComp = OwningCombatActor->GroggyComp;
	StatusComp = OwningCombatActor->StatusComp;
	ActionComp = OwningCombatActor->ActionComp;
	LocomotionComp = OwningCombatActor->LocomotionComp;
	HPComp = OwningCombatActor->HPComp;
}

void UCombatCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCombatActor || !MovementComp)
	{
		return;
	}

	const FVector Velocity = MovementComp->Velocity;
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	MovementDirection = CalculateDirection(Velocity, OwningCombatActor->GetActorRotation());
	bIsInAir = MovementComp->IsFalling();
	bIsAccelerating = MovementComp->GetCurrentAcceleration().SizeSquared() > 0.f;

	bIsGroggy = GroggyComp ? GroggyComp->bGroggy : false;

	if (MotionComp)
	{
		bIsCombatMotionActive = MotionComp->IsMotionActive();
		ActiveMotionType = MotionComp->GetActiveMotionType();
	}
	else
	{
		bIsCombatMotionActive = false;
		ActiveMotionType = EJRPGCombatMotionType::None;
	}

	bIsDead = HPComp ? HPComp->IsDead() : false;
}
