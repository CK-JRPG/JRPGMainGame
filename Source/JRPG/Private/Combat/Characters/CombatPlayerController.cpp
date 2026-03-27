#include "Combat/Characters/CombatPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Movement/LocomotionComponent.h"

#include "GameFramework/Character.h"

void ACombatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ACombatPlayerController::OnMove);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &ACombatPlayerController::OnMove);
	}

	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ACombatPlayerController::OnLook);
	}

	if (IA_CameraZoom)
	{
		EIC->BindAction(IA_CameraZoom, ETriggerEvent::Started, this, &ACombatPlayerController::OnCameraZoom);
	}
	
	if (IA_SwitchPrev)
	{
		EIC->BindAction(IA_SwitchPrev, ETriggerEvent::Started, this, &ACombatPlayerController::OnSwitchPrev);
	}

	if (IA_SwitchNext)
	{
		EIC->BindAction(IA_SwitchNext, ETriggerEvent::Started, this, &ACombatPlayerController::OnSwitchNext);
	}
}

void ACombatPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsys->ClearAllMappings();
			if (IMC_Combat)
			{
				Subsys->AddMappingContext(IMC_Combat, 0);
			}
		}
	}

	UpdateCameraTargetForPawn(InPawn);
}

void ACombatPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

//------Input------

void ACombatPlayerController::OnMove(const FInputActionValue& Value)
{
	const FVector2D Move = Value.Get<FVector2D>();

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	if (ULocomotionComponent* Locomotion = ControlledPawn->FindComponentByClass<ULocomotionComponent>())
	{
		Locomotion->SetMoveInput(Move);
		return;
	}

	if (ACharacter* Char = Cast<ACharacter>(ControlledPawn))
	{
		const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		Char->AddMovementInput(Forward, Move.Y);
		Char->AddMovementInput(Right, Move.X);
	}
}

void ACombatPlayerController::OnLook(const FInputActionValue& Value)
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

void ACombatPlayerController::OnSwitchPrev(const FInputActionValue& /*Value*/)
{
	SwitchCombatCharacter(-1);
}

void ACombatPlayerController::OnSwitchNext(const FInputActionValue& /*Value*/)
{
	SwitchCombatCharacter(1);
}

void ACombatPlayerController::SwitchCombatCharacter(int32 Direction)
{
	if (!GetWorld() || !GetGameInstance()) return;

	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	UPartySubsystem* PartySub = GetGameInstance()->GetSubsystem<UPartySubsystem>();

	if (!SpawnSub || !PartySub) return;

	const TArray<FName>& PartyIds = PartySub->GetPartyIds();
	if (PartyIds.Num() <= 1) return;

	const FName CurrentId = SpawnSub->GetCurrentPlayerCharacterID();
	if (CurrentId.IsNone()) return;

	const int32 CurrentIndex = PartyIds.IndexOfByKey(CurrentId);
	if (CurrentIndex == INDEX_NONE) return;

	const int32 NewIndex = (CurrentIndex + Direction + PartyIds.Num()) % PartyIds.Num();
	const FName NewId = PartyIds[NewIndex];

	SpawnSub->OnPartyMemberChanged(NewId);
}

void ACombatPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>(); // 휠업 = +1, 휠다운 = -1 (스틱도 동일 함)
	if (FMath::IsNearlyZero(AxisValue))
		return;
	
	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		// ZoomStep은 CameraRigActor 기준
		// 스틱은 0~1 연속값이라 스텝 곱해서 전달 했음
		// 부호 : 위로 댕기면 + = 줌인 ArmLength 감소
		CamSub->AdjustZoom(-AxisValue);
	}
}

void ACombatPlayerController::UpdateCameraTargetForPawn(APawn* InPawn) const
{
	if (!GetWorld()) return;

	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		CamSub->SetTarget(InPawn);
	}
}
