#include "Combat/Characters/CombatPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Characters/CombatTransitionSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Battle/CombatTargetingSubsystem.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"
#include "UI/JRPGHUD.h"

#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

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

	if (IA_TargetLockOn)
	{
		EIC->BindAction(IA_TargetLockOn, ETriggerEvent::Started, this, &ACombatPlayerController::OnTargetLockOn);
	}
	
	if (IA_SwitchPrev)
	{
		EIC->BindAction(IA_SwitchPrev, ETriggerEvent::Started, this, &ACombatPlayerController::OnSwitchPrev);
	}

	if (IA_SwitchNext)
	{
		EIC->BindAction(IA_SwitchNext, ETriggerEvent::Started, this, &ACombatPlayerController::OnSwitchNext);
	}
	
	if (IA_TacticalMode)
	{
		EIC->BindAction(IA_TacticalMode, ETriggerEvent::Started, this, &ACombatPlayerController::OnTacticalModePressed);
	}

	if (IA_ToggleMainMenu)
	{
		EIC->BindAction(IA_ToggleMainMenu, ETriggerEvent::Started, this, &ACombatPlayerController::OnToggleMainMenu);
	}

	if (IA_ToggleMainMenu)
	{
		EIC->BindAction(IA_ToggleMainMenu, ETriggerEvent::Started, this, &ACombatPlayerController::OnToggleMainMenu);
	}

	if (IA_Attack)
	{
		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ACombatPlayerController::OnBasicAttackMouseClick);
	}
}


void ACombatPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if(ULocalPlayer * LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsys->ClearAllMappings();
			UE_LOG(LogTemp, Log, TEXT("IMC_Combat 유효 여부: %s"), IMC_Combat ? TEXT("유효") : TEXT("NULL"));

			if (IMC_Combat)
				Subsys->AddMappingContext(IMC_Combat, 0);

		}
	}

	// 파티원 전환 시 적 포커싱 해제
	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		if (CamSub->IsLockedOn())
		{
			CamSub->ClearLockOn();
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
	// 락온 포커싱 중이면 카메라 회전 입력 차단
	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		if (CamSub->IsLockedOn())
			return;
	}

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

	UCombatTransitionSubsystem* TransitionSub = GetWorld()->GetSubsystem<UCombatTransitionSubsystem>();
	UPartySubsystem* PartySub = GetGameInstance()->GetSubsystem<UPartySubsystem>();

	if (!TransitionSub || !PartySub) return;

	const TArray<FName>& PartyIds = PartySub->GetPartyIds();
	if (PartyIds.Num() <= 1) return;

	const FName CurrentId = TransitionSub->GetCurrentPlayerCharacterID();
	if (CurrentId.IsNone()) return;

	const int32 CurrentIndex = PartyIds.IndexOfByKey(CurrentId);
	if (CurrentIndex == INDEX_NONE) return;

	const int32 NewIndex = (CurrentIndex + Direction + PartyIds.Num()) % PartyIds.Num();
	const FName NewId = PartyIds[NewIndex];

	TransitionSub->OnPartyMemberChanged(NewId);
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

void ACombatPlayerController::OnTargetLockOn(const FInputActionValue& Value)
{
	if (!GetWorld())
		return;

	UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>();
	if (!CamSub)
		return;

	if (CamSub->IsLockedOn())
	{
		// 이미 포커싱 중이면 다음 적으로 순환
		CamSub->ClearLockOn();
	}
	else
	{
		// 포커싱 시작 (가장 가까운 적)
		CamSub->LockOnEnemy();
	}
}

void ACombatPlayerController::UpdateCameraTargetForPawn(APawn* InPawn) const
{
	if (!GetWorld()) return;

	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		CamSub->SetTargetSmooth(InPawn);
	}
}

void ACombatPlayerController::OnTacticalModePressed(const FInputActionValue& Value)
{
	if (!GetWorld()) return;

	// Source/JRPG에 위치한 TacticalModeSubsystem 호출
	UTacticalModeSubsystem* TacticalSub = GetWorld()->GetSubsystem<UTacticalModeSubsystem>();
	if (!TacticalSub) return;

	if (TacticalSub->IsActive())
	{
		// 이미 전술 모드라면 수동 종료
		TacticalSub->ExitTacticalMode(FName("Input.ManualExit"));
	}
	else
	{
		// 꺼져 있다면 현재 조종 중인 폰을 Requester로 하여 진입 시도
		bool bSuccess = TacticalSub->TryEnterTacticalMode(GetPawn(), FName("Input.Tab"));
		
		if (!bSuccess)
		{
			// Zone 밖이거나 조건이 안 맞아서 거부된 경우
			UE_LOG(LogTemp, Warning, TEXT("전술 모드 진입 거부됨! (전투 구역 외부이거나 권한 없음)"));
		}
	}
}

void ACombatPlayerController::OnToggleMainMenu(const FInputActionValue& Value)
{
	if (AJRPGHUD* HUD = Cast<AJRPGHUD>(GetHUD()))
	{
		HUD->ToggleMainMenu();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JRPGPlayerController::OnToggleMainMenu: HUD가 존재하지 않음"));
	}
}



void ACombatPlayerController::OnBasicAttackMouseClick(const FInputActionValue& Value)
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnBasicAttackMouseClick] GetWorld() is null"));
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnBasicAttackMouseClick] ControlledPawn is null"));
		return;
	}

	UCombatPresentationComponent* Presentation = ControlledPawn->FindComponentByClass<UCombatPresentationComponent>();
	if (!Presentation)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnBasicAttackMouseClick] CombatPresentationComponent not found on pawn: %s"), *ControlledPawn->GetName());
		return;
	}

	UCombatTargetingSubsystem* Targeting = GetWorld()->GetSubsystem<UCombatTargetingSubsystem>();
	if (!Targeting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnBasicAttackMouseClick] CombatTargetingSubsystem not found"));
		return;
	}

	const FTargetingResult BasicTarget = Targeting->ResolvePreferredBasicAttackTarget(ControlledPawn);
	if (!BasicTarget.bOk || BasicTarget.Targets.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnBasicAttackMouseClick] No valid basic attack target found. bOk=%s, TargetCount=%d"),
			BasicTarget.bOk ? TEXT("true") : TEXT("false"),
			BasicTarget.Targets.Num());
		return;
	}

	if (AActor* Target = BasicTarget.Targets[0].Get())
	{
		UE_LOG(LogTemp, Log, TEXT("[OnBasicAttackMouseClick] Presenting basic attack on target: %s"), *Target->GetName());
		Presentation->TryPresentBasicAttack(Target);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnBasicAttackMouseClick] BasicTarget.Targets[0] is null (stale pointer)"));
	}
}