#include "Game/JRPGPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Game/JRPGPlayerPawn.h"
#include "Combat/Movement/LocomotionComponent.h"

void AJRPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMC_Default)
			{
				Subsys->AddMappingContext(IMC_Default, 0);
			}
		}
	}
}

void AJRPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AJRPGPlayerController::OnMove);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &AJRPGPlayerController::OnMove);
	}

	if (IA_Sprint)
	{
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AJRPGPlayerController::OnSprintStarted);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AJRPGPlayerController::OnSprintCompleted);
	}
	
	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AJRPGPlayerController::OnLook);
	}
}

void AJRPGPlayerController::OnMove(const FInputActionValue& Value)
{
	const FVector2D Move = Value.Get<FVector2D>();

	if (AJRPGPlayerPawn* P = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		if (P->Locomotion)
		{
			P->Locomotion->SetMoveInput(Move);
		}
	}
}

void AJRPGPlayerController::OnSprintStarted(const FInputActionValue& /*Value*/)
{
	if (AJRPGPlayerPawn* P = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		if (P->Locomotion) P->Locomotion->SetSprint(true);
	}
}

void AJRPGPlayerController::OnSprintCompleted(const FInputActionValue& /*Value*/)
{
	if (AJRPGPlayerPawn* P = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		if (P->Locomotion) P->Locomotion->SetSprint(false);
	}
}

void AJRPGPlayerController::OnLook(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if (LookAxisVector.X != 0.0f)
	{
		AddYawInput(LookAxisVector.X);
	}
	
	if (LookAxisVector.Y != 0.0f)
	{
		AddPitchInput(LookAxisVector.Y);
	}
}
