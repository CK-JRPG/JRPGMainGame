#include "Game/JRPGPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"

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
	

	UpdateCameraTargetForPawn(GetPawn());
	EnsureDefaultPartyFromTable();
	InitallizeCombatBridge();
}

void AJRPGPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	// 필드 IMC 복원 (전투 종료 후 컨트롤러 스왑 복귀 시 필요)
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsys->ClearAllMappings();
			if (IMC_Default)
			{
				Subsys->AddMappingContext(IMC_Default, 0);
			}
		}
	}
	
	UpdateCameraTargetForPawn(InPawn);
}

void AJRPGPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AJRPGPlayerController::EnsureDefaultPartyFromTable()
{
	if (!IsValid(CharacterTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bridge : CharacterTable이 설정되지 않음. DefaultPartyIds도 무시됨."));
		return;
	}
	
	UPartySubsystem* PartySys = GetGameInstance()->GetSubsystem<UPartySubsystem>();
	if (!PartySys)
		return;
	
	if (PartySys->GetPartyIds().Num() == 3)
	{
		UE_LOG(LogTemp, Log, TEXT("Bridge : 이미 파티 데이터가 존재하기 때문에 자동으로 초기화하지 않음."));
		return;
	}
	
	TArray<FName> AllRowNames = CharacterTable->GetRowNames();
	if (AllRowNames.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bridge: CharacterTable에 Row가 %d개밖에 없음. 최소 3개 필요."), AllRowNames.Num());
		return;
	}

	// DefaultPartyIds가 에디터에서 지정되어 있으면 그걸 사용하고 없으면 테이블 첫 3개 가져와서 사용해야함.
	TArray<FName> PartyToSet;
	if (DefaultPartyIds.Num() == 3)
	{
		PartyToSet = DefaultPartyIds;
		UE_LOG(LogTemp, Log, TEXT("Bridge: DefaultPartyIds 사용."));
	}
	else
	{
		PartyToSet = { AllRowNames[0], AllRowNames[1], AllRowNames[2] };
		UE_LOG(LogTemp, Log, TEXT("Bridge: CharacterTable 첫 3개 Row를 기본 파티로 사용."));
	}

	const bool bOk = PartySys->SetPartyIds(PartyToSet, "Init.DefaultParty");
	if (bOk)
	{
		UE_LOG(LogTemp, Log, TEXT("Bridge: 기본 파티 설정 완료 [%s, %s, %s]"),
			*PartyToSet[0].ToString(), *PartyToSet[1].ToString(), *PartyToSet[2].ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Bridge: 기본 파티 설정 실패."));
	}
}

void AJRPGPlayerController::InitallizeCombatBridge()
{
	UPartySubsystem* PartySubsystem = GetGameInstance()->GetSubsystem<UPartySubsystem>();
	if (!PartySubsystem) return;
 
	UPartyActorSpawnSubsystem* SpawnSub   = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	UCharacterRuntimeSubsystem* CharacterRuntime = GetGameInstance()->GetSubsystem<UCharacterRuntimeSubsystem>();
 
	if (!SpawnSub || !CharacterRuntime) return;

	// BP에서 설정한 전투 전용 컨트롤러 클래스 전달
	if (CombatControllerClass)
	{
		SpawnSub->SetCombatControllerClass(CombatControllerClass);
	}

	const TArray<FName>& CharIds = PartySubsystem->GetPartyIds();
	FName LeaderId = CharIds.Num() > 0 ? CharIds[0] : NAME_None;

	for (const FName& CharId : CharIds)
	{
		FCharacterMappingRow* Row = FindMappingRowById(CharId);
		if (!Row)
		{
			UE_LOG(LogTemp, Warning, TEXT("Bridge : [%s] MappingRow 없음. 스킵."), *CharId.ToString());
			continue;
		}
 
		if (!Row->CharacterAsset)
		{
			UE_LOG(LogTemp, Warning, TEXT("Bridge : [%s] CharacterAsset 없음. 스킵."), *CharId.ToString());
			continue;
		}
 
		//인카운터 클래스 등록
		FCharacterSpawnEntry Entry;
		Entry.CharacterID  = CharId;
		Entry.ActorClass   = Row->CombatActorClass;
		Entry.SpawnOffset  = Row->SpawnOffset;
		Entry.FieldPawnClass = Row->FieldPawnClass;
		SpawnSub->RegisterSpawnEntry(Entry);
 
		// 스냅샷 초기화로 데이터 에셋에서 직접 읽기
		const FCharacterBaseParams& P = Row->CharacterAsset->BaseParams;
		CharacterRuntime->InitializeSnapshotIfAbsent(CharId, P.MaxHP, P.MaxAP, P.MaxSP);
 
		UE_LOG(LogTemp, Log, TEXT("Bridge : [%s] 등록 완료. HP=%.1f AP=%d SP=%d"),
			*CharId.ToString(), P.MaxHP, P.MaxAP, P.MaxSP);
	}
	
	if (APawn* PlayerPawn = GetPawn())
		SpawnSub->SpawnFieldCompanions(PlayerPawn->GetActorLocation(), LeaderId);
 
	// PlayerPawn에 어태커(주인공)캐릭터 ID 연결
	if (AJRPGPlayerPawn* PlayerPawn = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		if (CharIds.Num() > 0 && PlayerPawn->CurrentCharacterId.IsNone())
		{
			PlayerPawn->UpdateCharacter(CharIds[0]);
			UE_LOG(LogTemp, Log, TEXT("Bridge: PlayerPawn 리드 캐릭터 → %s"), *CharIds[0].ToString());
		}
	}
}

UCombatCharacterDataAsset* AJRPGPlayerController::FindCharacterDefById(FName CharId) const
{
	FCharacterMappingRow* Row = FindMappingRowById(CharId);
	return Row ? Row->CharacterAsset : nullptr;
}

FCharacterMappingRow* AJRPGPlayerController::FindMappingRowById(FName CharId) const
{
	if (!IsValid(CharacterTable)) return nullptr;
	return CharacterTable->FindRow<FCharacterMappingRow>(CharId, TEXT("FindMappingRowById"));
}


//------Input------

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
	
	if (IA_LookAround)
	{
		EIC->BindAction(IA_LookAround, ETriggerEvent::Started, this, &AJRPGPlayerController::OnLookAround);
		EIC->BindAction(IA_LookAround, ETriggerEvent::Completed, this, &AJRPGPlayerController::OnLookAroundCompleted);
	}
}

void AJRPGPlayerController::OnMove(const FInputActionValue& Value)
{
	const FVector2D Move = Value.Get<FVector2D>();

	APawn* ControlledPawn = GetPawn(); 
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

void AJRPGPlayerController::OnSprintStarted(const FInputActionValue& /*Value*/)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (ULocomotionComponent* Locomotion = ControlledPawn->FindComponentByClass<ULocomotionComponent>())
		{
			Locomotion->SetSprint(true);
		}
	}
}

void AJRPGPlayerController::OnSprintCompleted(const FInputActionValue& /*Value*/)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (ULocomotionComponent* Locomotion = ControlledPawn->FindComponentByClass<ULocomotionComponent>())
		{
			Locomotion->SetSprint(false);
		}
	}
}

void AJRPGPlayerController::OnLook(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if (LookAxisVector.X != 0.0f)
	{
		AddYawInput(LookAxisVector.X * LookSensitivityX);
	}
	
	if (LookAxisVector.Y != 0.0f)
	{
		AddPitchInput(LookAxisVector.Y * LookSensitivityY);
	}
}

void AJRPGPlayerController::OnLookAround()
{
	APawn* ControlledPawn = GetPawn(); 
	if (ULocomotionComponent* Locomotion = ControlledPawn->FindComponentByClass<ULocomotionComponent>())
	{
		Locomotion->SetLookAroundMode(true);
	}
}

void AJRPGPlayerController::OnLookAroundCompleted()
{
	APawn* ControlledPawn = GetPawn(); 
	if (ULocomotionComponent* Locomotion = ControlledPawn->FindComponentByClass<ULocomotionComponent>())
	{
		Locomotion->SetLookAroundMode(false);
	}
}

void AJRPGPlayerController::UpdateCameraTargetForPawn(APawn* InPawn) const
{
	if (!GetWorld()) return;
	
	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		CamSub->SetTarget(InPawn);
	}
}
