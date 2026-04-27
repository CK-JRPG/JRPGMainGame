#include "Game/JRPGPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Game/PartySetupService.h"

#include "Game/JRPGPlayerPawn.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "UI/Presenters/InventoryPresenter.h"
#include "Combat/Items/InventorySubsystem.h"
#include "UI/JRPGHUD.h"
#include "UI/Presenters/ExplorationHUDPresenter.h"
#include "Game/HubSubsystem.h"
#include "Game/Companion/FieldCompanionSubsystem.h"

void AJRPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UpdateCameraTargetForPawn(GetPawn());
	EnsureDefaultPartyFromTable();

	if (UPartySubsystem* PartySys = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPartySubsystem>() : nullptr)
	{
		PartySys->OnPartyIdsChanged.RemoveAll(this);
		PartySys->OnPartyIdsChanged.AddUObject(this, &AJRPGPlayerController::HandlePartyIdsChanged);
	}

	InitallizeCombatBridge();


	// Todo - 해당 코드 에러 발생함 고쳐야 함
	// SubclassOf.h(111,38): Error C2027 : 정의되지 않은 형식 'UUserWidget'을(를) 사용했습니다.
	// 0>SubclassOf.h(111,38): Error C2672 : 'StaticClass': 일치하는 오버로드된 함수가 없습니다.
	// 위와 같은 오류 발생
	/*if (InventoryWidgetClass)
	{
		InventoryPresenter = NewObject<UInventoryPresenter>(this);
		InventoryPresenter->Initialize(GetWorld(), InventoryWidgetClass);
	}*/

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
	// 빙의 해제 전 폰의 이동 입력 상태 초기화 (전투 전환 시 잔존 입력 방지)
	if (APawn* pawn = GetPawn())
	{
		if (ULocomotionComponent* Loco = pawn->FindComponentByClass<ULocomotionComponent>())
		{
			Loco->SetMoveInput(FVector2D::ZeroVector);
			Loco->SetSprint(false);
		}
	}

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
	
	const TArray<FName>& ExistingPartyIds = PartySys->GetPartyIds();
	if (!ExistingPartyIds.IsEmpty())
	{
		bool bExistingPartyIsValid = true;
		for (const FName& CharacterId : ExistingPartyIds)
		{
			if (!IsValidPartyCharacterId(CharacterId))
			{
				UE_LOG(LogTemp, Warning, TEXT("Bridge : 저장된 파티 ID가 CharacterTable에 없음. 기본 파티로 재설정합니다. InvalidId=%s"),
					*CharacterId.ToString());
				bExistingPartyIsValid = false;
				break;
			}
		}

		if (bExistingPartyIsValid)
		{
			UE_LOG(LogTemp, Log, TEXT("Bridge : 이미 파티 데이터가 존재하기 때문에 자동으로 초기화하지 않음."));
			return;
		}
	}
	
	TArray<FName> AllRowNames = CharacterTable->GetRowNames();
	if (AllRowNames.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bridge: CharacterTable에 Row가 없음."));
		return;
	}

	// DefaultPartyIds가 에디터에서 지정되어 있으면 그걸 사용하고 없으면 테이블 첫 Row를 리더로 사용.
	TArray<FName> PartyToSet;
	for (const FName& CharacterId : DefaultPartyIds)
	{
		if (CharacterId.IsNone() || PartyToSet.Contains(CharacterId))
		{
			continue;
		}

		if (!IsValidPartyCharacterId(CharacterId))
		{
			UE_LOG(LogTemp, Warning, TEXT("Bridge: DefaultPartyIds의 ID가 CharacterTable에 없음. 스킵 - %s"), *CharacterId.ToString());
			continue;
		}

		PartyToSet.Add(CharacterId);
		if (PartyToSet.Num() >= UPartySubsystem::MaxPartySize)
		{
			break;
		}
	}

	if (!PartyToSet.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("Bridge: DefaultPartyIds 사용. Count=%d"), PartyToSet.Num());
	}
	else
	{
		PartyToSet = { AllRowNames[0] };
		UE_LOG(LogTemp, Log, TEXT("Bridge: CharacterTable 첫 Row를 기본 파티(1인)로 사용."));
	}

	const bool bOk = PartySys->SetPartyIds(PartyToSet, "Init.DefaultParty");
	if (bOk)
	{
		FString PartySummary;
		for (int32 Idx = 0; Idx < PartyToSet.Num(); ++Idx)
		{
			if (Idx > 0)
			{
				PartySummary += TEXT(", ");
			}
			PartySummary += PartyToSet[Idx].ToString();
		}

		UE_LOG(LogTemp, Log, TEXT("Bridge: 기본 파티 설정 완료 [%s]"), *PartySummary);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Bridge: 기본 파티 설정 실패."));
	}
}

void AJRPGPlayerController::InitallizeCombatBridge()
{
	if (UPartySetupService* SetupService = GetWorld()->GetSubsystem<UPartySetupService>())
		SetupService->InitializeCombatBridge(CharacterTable, CombatControllerClass, GetPawn());
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

bool AJRPGPlayerController::IsValidPartyCharacterId(FName CharacterId) const
{
	if (CharacterId.IsNone())
	{
		return false;
	}

	const FCharacterMappingRow* Row = FindMappingRowById(CharacterId);
	return Row && Row->CharacterAsset && !Row->CombatActorClass.IsNull();
}

bool AJRPGPlayerController::FindPartyCharacterIdByRole(EJRPGPartyRole PartyRole, FName& OutCharacterId) const
{
	OutCharacterId = NAME_None;

	if (!IsValid(CharacterTable))
	{
		return false;
	}

	const TArray<FName> RowNames = CharacterTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		const FCharacterMappingRow* Row = CharacterTable->FindRow<FCharacterMappingRow>(RowName, TEXT("FindPartyCharacterIdByRole"));
		if (!Row || !Row->CharacterAsset || Row->CombatActorClass.IsNull())
		{
			continue;
		}

		if (Row->CharacterAsset->DefaultRole == PartyRole)
		{
			OutCharacterId = RowName;
			return true;
		}
	}

	return false;
}

bool AJRPGPlayerController::BuildDebugPartyPreset(int32 TargetSize, TArray<FName>& OutPartyIds) const
{
	OutPartyIds.Reset();

	if (TargetSize < 1 || TargetSize > UPartySubsystem::MaxPartySize)
	{
		return false;
	}

	auto AddRequiredRole = [this, &OutPartyIds](EJRPGPartyRole PartyRole, const TCHAR* RoleLabel) -> bool
	{
		FName CharacterId = NAME_None;
		if (!FindPartyCharacterIdByRole(PartyRole, CharacterId))
		{
			UE_LOG(LogTemp, Error, TEXT("DebugPartyPreset : CharacterTable에서 역할 [%s] 파티원을 찾지 못했습니다."),
				RoleLabel);
			return false;
		}

		if (!IsValidPartyCharacterId(CharacterId))
		{
			UE_LOG(LogTemp, Error, TEXT("DebugPartyPreset : 유효하지 않은 파티 ID입니다. CharacterTable/RowName 확인 필요 - %s"),
				*CharacterId.ToString());
			return false;
		}

		if (OutPartyIds.Contains(CharacterId))
		{
			UE_LOG(LogTemp, Error, TEXT("DebugPartyPreset : 중복 파티 ID입니다 - %s"), *CharacterId.ToString());
			return false;
		}

		OutPartyIds.Add(CharacterId);
		return true;
	};

	if (!AddRequiredRole(EJRPGPartyRole::Attacker, TEXT("Attacker")))
	{
		return false;
	}

	if (TargetSize >= 2 && !AddRequiredRole(EJRPGPartyRole::Defender, TEXT("Defender")))
	{
		return false;
	}

	if (TargetSize >= 3 && !AddRequiredRole(EJRPGPartyRole::Supporter, TEXT("Supporter")))
	{
		return false;
	}

	return true;
}

void AJRPGPlayerController::ApplyDebugPartyPreset(int32 TargetSize)
{
	if (!Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		UE_LOG(LogTemp, Warning, TEXT("DebugPartyPreset : 필드 PlayerPawn 상태에서만 파티 프리셋을 적용할 수 있습니다."));
		return;
	}

	if (UBattleSessionSubsystem* BattleSub = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
	{
		if (BattleSub->IsBattleActive())
		{
			UE_LOG(LogTemp, Warning, TEXT("DebugPartyPreset : 전투 중에는 필드 파티 프리셋을 변경하지 않습니다."));
			return;
		}
	}

	UPartySubsystem* PartySys = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPartySubsystem>() : nullptr;
	if (!PartySys)
	{
		UE_LOG(LogTemp, Error, TEXT("DebugPartyPreset : PartySubsystem이 없습니다."));
		return;
	}

	TArray<FName> PartyToSet;
	if (!BuildDebugPartyPreset(TargetSize, PartyToSet))
	{
		return;
	}

	if (!PartySys->SetPartyIds(PartyToSet, FName(TEXT("Debug.FieldPartyPreset"))))
	{
		UE_LOG(LogTemp, Error, TEXT("DebugPartyPreset : PartySubsystem::SetPartyIds 실패. Count=%d"), PartyToSet.Num());
		return;
	}

	FString PartySummary;
	for (int32 Idx = 0; Idx < PartyToSet.Num(); ++Idx)
	{
		if (Idx > 0)
		{
			PartySummary += TEXT(", ");
		}
		PartySummary += PartyToSet[Idx].ToString();
	}

	UE_LOG(LogTemp, Log, TEXT("DebugPartyPreset : 필드 파티 프리셋 적용 완료 [%s]"), *PartySummary);
}

void AJRPGPlayerController::RefreshExplorationPartyUI() const
{
	if (AJRPGHUD* HUD = Cast<AJRPGHUD>(GetHUD()))
	{
		if (UExplorationHUDPresenter* Presenter = HUD->GetExplorationPresenter())
		{
			Presenter->RefreshPartyStatusData();
		}
	}
}

void AJRPGPlayerController::HandlePartyIdsChanged(const TArray<FName>& NewPartyIds, FName ReasonTag)
{
	if (UBattleSessionSubsystem* BattleSub = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
	{
		if (BattleSub->IsBattleActive())
		{
			UE_LOG(LogTemp, Warning, TEXT("JRPGPlayerController : 전투 중 파티 변경 이벤트 무시 (Reason=%s)"), *ReasonTag.ToString());
			return;
		}
	}

	if (!Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		UE_LOG(LogTemp, Warning, TEXT("JRPGPlayerController : 필드 Pawn 상태가 아니라 파티 변경 후 브릿지 갱신을 보류 (Reason=%s)"), *ReasonTag.ToString());
		return;
	}

	InitallizeCombatBridge();
	RefreshExplorationPartyUI();

	UE_LOG(LogTemp, Log, TEXT("JRPGPlayerController : 파티 변경 반영 완료 Count=%d Reason=%s"), NewPartyIds.Num(), *ReasonTag.ToString());
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
	
	if (IA_CameraZoom)
	{
		EIC->BindAction(IA_CameraZoom, ETriggerEvent::Started, this, &AJRPGPlayerController::OnCameraZoom);
	}

	if (IA_ToggleMainMenu)
	{
		EIC->BindAction(IA_ToggleMainMenu, ETriggerEvent::Started, this, &AJRPGPlayerController::OnToggleMainMenu);
	}
	
	if (IA_Interact)
	{
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AJRPGPlayerController::OnInteract);
	}

	if (IA_TogglePartyStatus)
	{
		EIC->BindAction(IA_TogglePartyStatus, ETriggerEvent::Started, this, &AJRPGPlayerController::OnTogglePartyStatus);
	}

	if (IA_DebugPartyOne)
	{
		EIC->BindAction(IA_DebugPartyOne, ETriggerEvent::Started, this, &AJRPGPlayerController::OnDebugPartyOne);
	}

	if (IA_DebugPartyTwo)
	{
		EIC->BindAction(IA_DebugPartyTwo, ETriggerEvent::Started, this, &AJRPGPlayerController::OnDebugPartyTwo);
	}

	if (IA_DebugPartyThree)
	{
		EIC->BindAction(IA_DebugPartyThree, ETriggerEvent::Started, this, &AJRPGPlayerController::OnDebugPartyThree);
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

void AJRPGPlayerController::OnCameraZoom(const FInputActionValue& Value)
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

void AJRPGPlayerController::OnInteract()
{
	// 허브 상호작용 체크
	if (UHubSubsystem* HubSub = GetWorld()->GetSubsystem<UHubSubsystem>())
	{
		if (AActor* FocusedHub = HubSub->GetFocusedHub())
		{
			// 허브 액터 위치가 아닌 플레이어의 현재 위치를 리스폰 좌표로 저장
			APawn* PlayerPawn = GetPawn();
			if (!PlayerPawn)
			{
				UE_LOG(LogTemp, Warning, TEXT("JRPGPlayerController::OnInteract : Pawn이 없어 허브 방문 등록 불가"));
				return;
			}

			HubSub->VisitHub(FocusedHub, PlayerPawn->GetActorLocation());
			
			// 컴패니언 위치도 함께 스냅샷 저장 (패배 리스폰 시 사용)
			if (UFieldCompanionSubsystem* CompanionSub = GetWorld()->GetSubsystem<UFieldCompanionSubsystem>())
			{
				CompanionSub->SaveCompanionLocations();
			}
			return;
		}
	}
	
	// 향후 상자, NPC 등 다른 상호작용 대상 추가 가능
}

void AJRPGPlayerController::UpdateCameraTargetForPawn(APawn* InPawn) const
{
	if (!GetWorld()) return;
	
	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		CamSub->SetTarget(InPawn);
	}
}

void AJRPGPlayerController::OnToggleMainMenu()
{
	//UE_LOG(LogTemp, Warning, TEXT("JRPGPlayerController::OnToggleMainMenu: "));

	if (AJRPGHUD* HUD = Cast<AJRPGHUD>(GetHUD()))
	{
		HUD->ToggleMainMenu();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JRPGPlayerController::OnToggleMainMenu: HUD가 존재하지 않음"));
	}
}

void AJRPGPlayerController::OnTogglePartyStatus()
{
	if (AJRPGHUD* HUD = Cast<AJRPGHUD>(GetHUD()))
	{
		HUD->TogglePartyInfo();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JRPGPlayerController::OnToggleMainMenu: HUD가 존재하지 않음"));
	}

}

void AJRPGPlayerController::OnDebugPartyOne()
{
	ApplyDebugPartyPreset(1);
}

void AJRPGPlayerController::OnDebugPartyTwo()
{
	ApplyDebugPartyPreset(2);
}

void AJRPGPlayerController::OnDebugPartyThree()
{
	ApplyDebugPartyPreset(3);
}

AActor* AJRPGPlayerController::GetCheatTargetActor(FName CharacterId) const
{
	if (CharacterId.IsNone())
	{
		return GetPawn();
	}
	return GetPawn();
}

void AJRPGPlayerController::GiveItem(FName ItemId, int32 Count)
{
	if (UInventorySubsystem* InvSub = GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		InvSub->AddItem(ItemId, Count, FName("Cheat.GiveItem"));
		//UE_LOG(LogTemp, Warning, TEXT("[Cheat] %s 아이템을 %d개 지급했습니다."), *ItemId.ToString(), Count);
	}
}

void AJRPGPlayerController::ClearInventory()
{
	if (UInventorySubsystem* InvSub = GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		InvSub->ClearInventory();
		//UE_LOG(LogTemp, Warning, TEXT("[Cheat] 인벤토리를 모두 비웠습니다."));
	}
}

void AJRPGPlayerController::DumpInventory()
{
	if (UInventorySubsystem* InvSub = GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		TArray<FItemInstance> Items;
		InvSub->GetAllInstances(Items);

		UE_LOG(LogTemp, Warning, TEXT("=== [인벤토리 덤프] 총 %d개 ==="), Items.Num());
		for (const FItemInstance& Item : Items)
		{
			UE_LOG(LogTemp, Log, TEXT(" - [%s] %s (수량: %d)"), *Item.InstanceId.ToString(), *Item.ItemId.ToString(), Item.Quantity);
		}
	}
}

void AJRPGPlayerController::DumpEquipment(FName CharacterId)
{

}

void AJRPGPlayerController::ForceEquip(FName CharacterId, int32 Slot, FName ItemId)
{

}

void AJRPGPlayerController::RebuildStats(FName CharacterId)
{

}

//void AJRPGPlayerController::AutoEquipForRole(FName CharacterId, FName Role)
//{
//
//}
//
