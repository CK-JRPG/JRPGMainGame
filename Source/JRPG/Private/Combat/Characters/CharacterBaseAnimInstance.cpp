#include "Combat/Characters/CharacterBaseAnimInstance.h"
#include "GameFramework/Character.h"


void UCharacterBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	MovementComponent = OwnerCharacter->GetCharacterMovement();
}

void UCharacterBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateMovementVariables();
}

void UCharacterBaseAnimInstance::UpdateMovementVariables()
{
	if (!OwnerCharacter.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterBaseAnimInstance : OwnerCharacter is None"));
		return;
	}

	if (!MovementComponent.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterBaseAnimInstance : MovementComponent is None"));
		return;
	}
}
