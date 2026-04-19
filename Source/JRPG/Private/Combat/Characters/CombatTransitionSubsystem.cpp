#include "Combat/Characters/CombatTransitionSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Characters/CombatPlayerController.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Game/Companion/FieldCompanionSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/AI/CombatPartyAIComponent.h"
#include "Game/HubSubsystem.h"
#include "UI/JRPGHUD.h"
#include "UI/Presenters/ExplorationHUDPresenter.h"


void UCombatTransitionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UBattleSessionSubsystem* BattleSub = InWorld.GetSubsystem<UBattleSessionSubsystem>())
	{
		BattleSub->OnBattleEnded.AddUObject(this, &UCombatTransitionSubsystem::HandleBattleEnded);
	}

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 초기화 완료."));
}

void UCombatTransitionSubsystem::SetOriginalPlayerCharacterID(const FName& CharacterID)
{
	OriginalPlayerCharacterID = CharacterID;
	CurrentPlayerCharacterID  = CharacterID;

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 원본 플레이어 캐릭터 설정 - %s"), *CharacterID.ToString());
}

void UCombatTransitionSubsystem::SetCombatControllerClass(TSubclassOf<APlayerController> InClass)
{
	CombatControllerClass = InClass;
	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : CombatControllerClass 설정 → %s"), *GetNameSafe(InClass));
}

// ==== 전투 모드 진입 ====

void UCombatTransitionSubsystem::EnterCombatMode(APlayerController* PC, const FName& LeaderCharacterID)
{
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::EnterCombatMode : PlayerController가 없음."));
		return;
	}

	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	if (!SpawnSub)
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::EnterCombatMode : PartyActorSpawnSubsystem 없음."));
		return;
	}

	ACombatCharacterActor* LeaderActor = SpawnSub->FindActorByCharacterID(LeaderCharacterID);
	if (!LeaderActor)
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::EnterCombatMode : 리더 CombatCharacterActor를 찾을 수 없음 - %s"), *LeaderCharacterID.ToString());
		return;
	}

	// 상태 저장
	const FRotator FieldControlRotation = PC->GetControlRotation();
	CombatPlayerController = PC;
	SetOriginalPlayerCharacterID(LeaderCharacterID);
	CachedFieldPawn = PC->GetPawn();
	CachedFieldController = PC;

	// 카메라 스냅샷 저장 (전투 종료 후 복원용)
	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		CamSub->SaveFieldSnapshot();
	}

	// JRPGPlayerPawn HiddenInGame으로 변경하고 콜리전 false
	if (APawn* FieldPawn = CachedFieldPawn.Get())
	{
		FieldPawn->SetActorHiddenInGame(true);
		FieldPawn->SetActorEnableCollision(false);
		FieldPawn->SetActorTickEnabled(false);
	}
	PC->UnPossess();

	// CompanionPawn 숨기기 + AI 제거
	if (UFieldCompanionSubsystem* CompanionSub = GetWorld()->GetSubsystem<UFieldCompanionSubsystem>())
	{
		CompanionSub->HideCompanions();
	}

	// 리더 CombatCharacterActor의 기존 AI 컨트롤러 제거
	if (AController* ExistingAI = LeaderActor->GetController())
	{
		ExistingAI->UnPossess();
		ExistingAI->Destroy();
	}

	// CombatPlayerController 스폰 및 컨트롤러 스왑
	{
		UWorld* World = GetWorld();
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		UClass* ClassToSpawn = CombatControllerClass.Get() ? CombatControllerClass.Get() : ACombatPlayerController::StaticClass();

		APlayerController* CombatPC = World->SpawnActor<APlayerController>(ClassToSpawn, FTransform::Identity, SpawnParams);

		if (CombatPC)
		{
			// 필드 컨트롤러의 카메라 회전값을 전투 컨트롤러에 동기화 (끊김 방지)
			CombatPC->SetControlRotation(FieldControlRotation);

			// 필드 컨트롤러 → 전투 컨트롤러로 LocalPlayer 스왑
			if (AGameModeBase* GM = World->GetAuthGameMode())
			{
				GM->SwapPlayerControllers(PC, CombatPC);

				// 스왑 직후 전투 컨트롤러에 명시적으로 HUD 생성 명령
				if (GM->HUDClass)
				{
					CombatPC->ClientSetHUD(GM->HUDClass);
				}
			}

			CombatPC->Possess(LeaderActor);
			CombatPC->SetControlRotation(FieldControlRotation);
			CombatPlayerController = CombatPC;

			// 필드 폰 → 전투 액터 이동 속도/입력 동기화
			SyncMovementStateToLeader(CachedFieldPawn.Get(), LeaderActor);

			UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem::EnterCombatMode : CombatPlayerController 스왑 완료 → %s"), *LeaderCharacterID.ToString());
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("CombatTransitionSubsystem::EnterCombatMode : CombatPlayerController 스폰 실패. 필드 PC로 폴백."));
		}
	}

	// 폴백 : CombatPlayerController 스폰 실패 시 기존 필드 PC 사용
	PC->Possess(LeaderActor);
	PC->SetControlRotation(FieldControlRotation);
	CombatPlayerController = PC;

	// 폴백 경로에서도 필드 폰 → 전투 액터 이동 속도/입력 동기화
	SyncMovementStateToLeader(CachedFieldPawn.Get(), LeaderActor);

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem::EnterCombatMode : 전투 모드 진입 완료 (필드 PC 폴백) -> %s"), *LeaderCharacterID.ToString());
}

// ==== 이동 상태 동기화 ====

void UCombatTransitionSubsystem::SyncMovementStateToLeader(APawn* FieldPawn, ACombatCharacterActor* LeaderActor)
{
	if (!FieldPawn || !LeaderActor) return;

	if (ACharacter* FieldChar = Cast<ACharacter>(FieldPawn))
	{
		if (UCharacterMovementComponent* FieldCMC = FieldChar->GetCharacterMovement())
		{
			if (UCharacterMovementComponent* CombatCMC = LeaderActor->GetCharacterMovement())
			{
				CombatCMC->Velocity = FieldCMC->Velocity;
			}
		}
	}

	if (ULocomotionComponent* FieldLoco = FieldPawn->FindComponentByClass<ULocomotionComponent>())
	{
		FieldLoco->SetMoveInput(FVector2D::ZeroVector);
		FieldLoco->SetSprint(false);
	}

	// 전투 캐릭터의 입력도 제로로 초기화 (깨끗한 상태로 시작)
	if (ULocomotionComponent* CombatLoco = LeaderActor->FindComponentByClass<ULocomotionComponent>())
	{
		CombatLoco->SetMoveInput(FVector2D::ZeroVector);
		CombatLoco->SetSprint(false);
	}
}

void UCombatTransitionSubsystem::SyncMovementStateToFieldPawn(ACombatCharacterActor* LeaderActor, APawn* FieldPawn)
{
	if (!LeaderActor || !FieldPawn) return;

	if (ACharacter* FieldChar = Cast<ACharacter>(FieldPawn))
	{
		if (UCharacterMovementComponent* CombatCMC = LeaderActor->GetCharacterMovement())
		{
			if (UCharacterMovementComponent* FieldCMC = FieldChar->GetCharacterMovement())
			{
				FieldCMC->Velocity = CombatCMC->Velocity;
			}
		}
	}

	if (ULocomotionComponent* CombatLoco = LeaderActor->FindComponentByClass<ULocomotionComponent>())
	{
		if (ULocomotionComponent* FieldLoco = FieldPawn->FindComponentByClass<ULocomotionComponent>())
		{
			FieldLoco->SetMoveInput(CombatLoco->GetMoveInput());
			FieldLoco->SetSprint(CombatLoco->IsSprinting());
		}
	}
}

// ==== 전투 중 조작 캐릭터 전환 ====

void UCombatTransitionSubsystem::OnPartyMemberChanged(const FName& NewCharacterID)
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	if (!BattleSession || !BattleSession->IsBattleActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatTransitionSubsystem : 필드 상태에서 조작 캐릭터 전환 불가."));
		return;
	}

	if (CurrentPlayerCharacterID == NewCharacterID) return;

	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	if (!SpawnSub) return;

	ACombatCharacterActor* TargetActor = SpawnSub->FindActorByCharacterID(NewCharacterID);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem : 전환 대상 Actor 없음 - %s"), *NewCharacterID.ToString());
		return;
	}

	if (!CombatPlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem : CombatPlayerController 미등록."));
		return;
	}

	if (AController* ExistingAI = TargetActor->GetController())
	{
		if (!ExistingAI->IsPlayerController())
		{
			ExistingAI->UnPossess();
			ExistingAI->Destroy();
		}
	}

	APawn* OldPawn = CombatPlayerController->GetPawn();

	CombatPlayerController->UnPossess();

	if (IsValid(OldPawn))
	{
		if (ULocomotionComponent* OldLoco = OldPawn->FindComponentByClass<ULocomotionComponent>())
		{
			OldLoco->SetMoveInput(FVector2D::ZeroVector);
			OldLoco->SetSprint(false);
		}

		if (ACharacter* OldCharacter = Cast<ACharacter>(OldPawn))
		{
			if (UCharacterMovementComponent* OldMovement = OldCharacter->GetCharacterMovement())
			{
				OldMovement->StopMovementImmediately();
			}
		}
		
		if (UCombatPartyAIComponent* OldPartyAI = OldPawn->FindComponentByClass<UCombatPartyAIComponent>())
		{
			OldPartyAI->SetComponentTickEnabled(true);
		}		
		OldPawn->SpawnDefaultController();
	}

	CombatPlayerController->Possess(TargetActor);
	
	if (UCombatPartyAIComponent* NewPartyAI = TargetActor->FindComponentByClass<UCombatPartyAIComponent>())
	{
		NewPartyAI->SetComponentTickEnabled(true);
	}	
	
	CurrentPlayerCharacterID = NewCharacterID;

	OnPartyMemberChangedDelegate.Broadcast(NewCharacterID);

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 빙의 전환 완료 → %s"), *NewCharacterID.ToString());
}

// ==== 전투 종료 처리 ====

void UCombatTransitionSubsystem::OnBattleEnded(EBattleEndReason Reason)
{
	if (OriginalPlayerCharacterID.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::OnBattleEnded : OriginalPlayerCharacterID 미설정."));
		return;
	}

	bIsTransitioning = true;

	if (Reason == EBattleEndReason::Victory)
	{
		HandleVictoryTransition();
	}
	else if (Reason == EBattleEndReason::Defeat)
	{
		HandleDefeatTransition();
	}
	else
	{
		// Aborted 등 기타: 즉시 전환
		PerformTransition(true);
		ResetTransitionState();
	}
}

// ==== 승리 처리 ====

void UCombatTransitionSubsystem::HandleVictoryTransition()
{
	// 승리 시: 전투 필드 액터 위치 그대로 필드로 복귀 (실시간 전환)
	PerformTransition(true);
	StartPostBattleRecovery();
	ResetTransitionState();

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 승리 전환 완료. 필드 모드 복원."));
}

// ==== 패배 처리 ====

void UCombatTransitionSubsystem::HandleDefeatTransition()
{
	// 플레이어 이동 처리 막기
	if (IsValid(CombatPlayerController))
	{
		CombatPlayerController->SetIgnoreMoveInput(true);
		CombatPlayerController->SetIgnoreLookInput(true);
	}

	// 2초 페이드 아웃
	if (IsValid(CombatPlayerController))
	{
		if (APlayerCameraManager* CamMgr = CombatPlayerController->PlayerCameraManager)
		{
			CamMgr->StartCameraFade(0.f, 1.f, 2.f, FLinearColor::Black, false, true);
		}
	}

	// 페이드 아웃 완료 후 전환 타이머
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DefeatFadeOutTimerHandle, this,
			&UCombatTransitionSubsystem::OnDefeatFadeOutComplete, 2.0f, false);
	}

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 패배 감지. 페이드 아웃 시작 (2초)."));
}

void UCombatTransitionSubsystem::OnDefeatFadeOutComplete()
{
	// 전투 종료 직전 카메라 상태를 필드 복귀용 스냅샷으로 갱신
	if (UCameraSubsystem* CamSub = GetWorld() ? GetWorld()->GetSubsystem<UCameraSubsystem>() : nullptr)
	{
		CamSub->SaveFieldSnapshot(CachedFieldPawn.Get());
	}

	// 주인공 캐릭터에 빙의 복귀
	ReturnPossessionToLeader();

	// CombatCharacterActor에서 빙의 해제 (패배 시 위치는 리더 위치가 아닌 허브로 이동)
	if (IsValid(CombatPlayerController))
	{
		CombatPlayerController->UnPossess();
	}

	// 전투 액터 -> 필드 폰 이동 동기화 (파괴 전에 수행)
	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	if (SpawnSub && IsValid(CachedFieldPawn))
	{
		ACombatCharacterActor* LeaderForSync = SpawnSub->FindActorByCharacterID(OriginalPlayerCharacterID);
		if (LeaderForSync)
		{
			SyncMovementStateToFieldPawn(LeaderForSync, CachedFieldPawn.Get());
		}
	}

	// CombatCharacterActor 스냅샷 저장 후 파괴
	if (SpawnSub)
	{
		TArray<ACombatCharacterActor*> CurrentActors = SpawnSub->GetSpawnedActors();
		SpawnSub->DespawnCombatActors(CurrentActors);
	}

	// 사망 캐릭터 부활 + HP 100% 회복
	HandleDefeatRecovery();

	// 컨트롤러 스왑 복원
	APlayerController* ControllerToRestore = nullptr;
	APlayerController* CombatPCToDestroy = nullptr;
	RestoreFieldController(ControllerToRestore, CombatPCToDestroy);

	// HubSubsystem에서 리스폰 위치 조회 (마지막 방문 허브 우선, 없으면 가장 가까운 허브)
	FVector HubLocation = FVector::ZeroVector;
	if (UHubSubsystem* HubSub = GetWorld()->GetSubsystem<UHubSubsystem>())
	{
		const FVector FallbackOrigin = IsValid(CachedFieldPawn) ? CachedFieldPawn->GetActorLocation() : FVector::ZeroVector;
		HubLocation = HubSub->GetRespawnLocation(FallbackOrigin);
	}

	// 필드 폰 복원 (허브 위치로)
	RestoreFieldPawn(ControllerToRestore, HubLocation, FRotator::ZeroRotator, true);

	// 필드 컨트롤러/폰 복원 완료 후 카메라 복원
	if (UCameraSubsystem* CamSub = GetWorld() ? GetWorld()->GetSubsystem<UCameraSubsystem>() : nullptr)
	{
		CamSub->RestoreFieldSnapshot();
	}
	
	// 전투 컨트롤러 파괴
	if (CombatPCToDestroy)
	{
		CombatPCToDestroy->Destroy();
	}

	// CompanionPawn 복원 (저장된 허브 방문 시 위치로 복원)
	if (UFieldCompanionSubsystem* CompanionSub = GetWorld()->GetSubsystem<UFieldCompanionSubsystem>())
	{
		CompanionSub->RestoreCompanionsToSavedLocations();
	}

	// 필드 컨트롤러 이동 입력 막기 (페이드 인 완료 전까지)
	DefeatRestoredController = ControllerToRestore;
	if (IsValid(DefeatRestoredController))
	{
		DefeatRestoredController->SetIgnoreMoveInput(true);
		DefeatRestoredController->SetIgnoreLookInput(true);
	}

	// 2초 페이드 인
	if (IsValid(ControllerToRestore))
	{
		if (APlayerCameraManager* CamMgr = ControllerToRestore->PlayerCameraManager)
		{
			CamMgr->StartCameraFade(1.f, 0.f, 2.f, FLinearColor::Black, false, false);
		}
	}

	// 페이드 인 완료 타이머
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DefeatFadeInTimerHandle, this,
			&UCombatTransitionSubsystem::OnDefeatFadeInComplete, 2.0f, false);
	}

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 패배 전환 완료. 허브로 이동 후 페이드 인 시작 (2초)."));
}

void UCombatTransitionSubsystem::OnDefeatFadeInComplete()
{
	// 이동 입력 복원
	if (IsValid(DefeatRestoredController))
	{
		DefeatRestoredController->SetIgnoreMoveInput(false);
		DefeatRestoredController->SetIgnoreLookInput(false);
		DefeatRestoredController = nullptr;
	}

	ResetTransitionState();

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 패배 처리 완료. 필드 모드 복원."));
}

// ==== 승리 후 점진적 HP 회복 ====
void UCombatTransitionSubsystem::StartPostBattleRecovery()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PostBattleRecoveryTimerHandle, this,
			&UCombatTransitionSubsystem::TickPostBattleRecovery,
			PostBattleRecoveryInterval, true);
	}

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 승리 후 점진적 HP 회복 시작 (%.1f초 간격, MaxHP의 %.0f%% 회복)."),
		PostBattleRecoveryInterval, PostBattleRecoveryRatio * 100.f);

	// UI 호출
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AJRPGHUD* HUD = Cast<AJRPGHUD>(PC->GetHUD()))
		{
			if (UExplorationHUDPresenter* Presenter = HUD->GetExplorationPresenter())
			{
				Presenter->StartPostCombatRegenUI();
			}
		}
	}
}

void UCombatTransitionSubsystem::TickPostBattleRecovery()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;

	UCharacterRuntimeSubsystem* CharRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>();
	if (!CharRuntime) return;

	const bool bStillRecovering = CharRuntime->RecoverPartyAfterVictory(PostBattleRecoveryRatio);

	if (!bStillRecovering)
	{
		World->GetTimerManager().ClearTimer(PostBattleRecoveryTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 승리 후 HP 회복 완료."));
	}
}

// ==== 공통 전환 로직 (승리) ====

void UCombatTransitionSubsystem::PerformTransition(bool bUseLeaderPosition)
{
	// 전투 종료 직전 카메라 상태를 필드 복귀용 스냅샷으로 갱신
	if (UCameraSubsystem* CamSub = GetWorld() ? GetWorld()->GetSubsystem<UCameraSubsystem>() : nullptr)
	{
		CamSub->SaveFieldSnapshot(CachedFieldPawn.Get());
	}

	// 주인공 캐릭터에 빙의 복귀
	ReturnPossessionToLeader();

	// 리더 트랜스폼/이동 동기화 저장
	FVector LeaderFinalLocation;
	FRotator LeaderFinalRotation;
	bool bHasLeaderTransform = false;
	if (bUseLeaderPosition)
	{
		SaveLeaderTransformAndSync(LeaderFinalLocation, LeaderFinalRotation, bHasLeaderTransform);
	}
	else if (IsValid(CombatPlayerController))
	{
		CombatPlayerController->UnPossess();
	}

	// CombatCharacterActor 스냅샷 저장 후 파괴
	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	if (SpawnSub)
	{
		TArray<ACombatCharacterActor*> CurrentActors = SpawnSub->GetSpawnedActors();
		SpawnSub->DespawnCombatActors(CurrentActors);
	}

	// 컨트롤러 스왑 복원
	APlayerController* ControllerToRestore = nullptr;
	APlayerController* CombatPCToDestroy = nullptr;
	RestoreFieldController(ControllerToRestore, CombatPCToDestroy);

	// 필드 폰 복원
	RestoreFieldPawn(ControllerToRestore, LeaderFinalLocation, LeaderFinalRotation, bHasLeaderTransform);

	// 필드 컨트롤러/폰 복원 완료 후 카메라 복원
	if (UCameraSubsystem* CamSub = GetWorld() ? GetWorld()->GetSubsystem<UCameraSubsystem>() : nullptr)
	{
		CamSub->RestoreFieldSnapshot();
	}

	// 전투 컨트롤러 파괴
	if (CombatPCToDestroy)
	{
		CombatPCToDestroy->Destroy();
	}

	// CompanionPawn 복원
	if (UFieldCompanionSubsystem* CompanionSub = GetWorld()->GetSubsystem<UFieldCompanionSubsystem>())
	{
		CompanionSub->RestoreCompanions();
	}
}

// ==== 서브 함수 ====

void UCombatTransitionSubsystem::ReturnPossessionToLeader()
{
	if (CurrentPlayerCharacterID == OriginalPlayerCharacterID || !CombatPlayerController)
		return;

	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	if (!SpawnSub) return;

	ACombatCharacterActor* LeaderActor = SpawnSub->FindActorByCharacterID(OriginalPlayerCharacterID);
	if (!LeaderActor) return;

	// 현재 빙의 중인 캐릭터 해제 → AI 복원
	APawn* OldPawn = CombatPlayerController->GetPawn();
	CombatPlayerController->UnPossess();
	if (IsValid(OldPawn))
	{
		OldPawn->SpawnDefaultController();
	}

	// 리더의 기존 AI 제거 후 플레이어 빙의
	if (AController* ExistingAI = LeaderActor->GetController())
	{
		if (!ExistingAI->IsPlayerController())
		{
			ExistingAI->UnPossess();
			ExistingAI->Destroy();
		}
	}
	CombatPlayerController->Possess(LeaderActor);
	CurrentPlayerCharacterID = OriginalPlayerCharacterID;

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem::OnBattleEnded : 주인공 캐릭터에 카메라 복귀 완료 -> %s"), *OriginalPlayerCharacterID.ToString());
}

void UCombatTransitionSubsystem::SaveLeaderTransformAndSync(FVector& OutLocation, FRotator& OutRotation, bool& bOutValid)
{
	bOutValid = false;

	// CombatCharacterActor에서 빙의 해제
	if (CombatPlayerController)
	{
		CombatPlayerController->UnPossess();
	}

	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	if (!SpawnSub) return;

	ACombatCharacterActor* LeaderForRestore = SpawnSub->FindActorByCharacterID(OriginalPlayerCharacterID);
	if (!LeaderForRestore) return;

	OutLocation = LeaderForRestore->GetActorLocation();
	OutRotation = LeaderForRestore->GetActorRotation();
	bOutValid = true;

	// 전투 액터 → 필드 폰 이동 속도/입력 동기화 (파괴 전에 수행)
	if (IsValid(CachedFieldPawn))
	{
		SyncMovementStateToFieldPawn(LeaderForRestore, CachedFieldPawn.Get());
	}
}

void UCombatTransitionSubsystem::HandleDefeatRecovery()
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UCharacterRuntimeSubsystem* CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>())
			{
				CharacterRuntime->RecoverPartyFromWipe(1.0f, 1.0f);
			}
		}
	}
}

void UCombatTransitionSubsystem::RestoreFieldController(APlayerController*& OutControllerToRestore, APlayerController*& OutCombatPCToDestroy)
{
	OutControllerToRestore = nullptr;
	OutCombatPCToDestroy = nullptr;

	FRotator RestoredControlRotation = FRotator::ZeroRotator;
	bool bHasRestoredControlRotation = false;

	if (UCameraSubsystem* CamSub = GetWorld() ? GetWorld()->GetSubsystem<UCameraSubsystem>() : nullptr)
	{
		bHasRestoredControlRotation = CamSub->GetSavedFieldControlRotation(RestoredControlRotation);
	}
	if (!bHasRestoredControlRotation && IsValid(CombatPlayerController))
	{
		RestoredControlRotation = CombatPlayerController->GetControlRotation();
		bHasRestoredControlRotation = true;
		UE_LOG(LogTemp, Warning, TEXT("CombatTransitionSubsystem::RestoreFieldController : 카메라 스냅샷 회전이 없어 CombatPlayerController 회전으로 폴백합니다."));
	}

	if (IsValid(CachedFieldController) && IsValid(CombatPlayerController) && CombatPlayerController != CachedFieldController)
	{
		if (bHasRestoredControlRotation)
		{
			CachedFieldController->SetControlRotation(RestoredControlRotation);
		}

		if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
		{
			GM->SwapPlayerControllers(CombatPlayerController.Get(), CachedFieldController.Get());

			if (GM->HUDClass)
			{
				CachedFieldController->ClientSetHUD(GM->HUDClass);
			}
		}
		OutControllerToRestore = CachedFieldController.Get();
		OutCombatPCToDestroy = CombatPlayerController.Get();

		UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem::OnBattleEnded : CombatPlayerController -> 필드 컨트롤러 스왑 복원."));
	}
	else if (!IsValid(CachedFieldController) && IsValid(CombatPlayerController)) 
	{
		// CachedFieldController가 비동기 패배 흐름 중 무효화된 경우:
		// GameMode의 PlayerControllerClass로 새 필드 컨트롤러를 스폰하여 복원
		UWorld* World = GetWorld();
		AGameModeBase* GM = World ? World->GetAuthGameMode() : nullptr;

		if (GM && GM->PlayerControllerClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			APlayerController* NewFieldPC = World->SpawnActor<APlayerController>(GM->PlayerControllerClass, FTransform::Identity, SpawnParams);
			if (NewFieldPC)
			{
				if (bHasRestoredControlRotation)
				{
					NewFieldPC->SetControlRotation(RestoredControlRotation);
				}

				// SwapPlayerControllers는 LocalPlayer 연결을 이전하고 기존 컨트롤러를 분리함
				GM->SwapPlayerControllers(CombatPlayerController.Get(), NewFieldPC);

				if (GM->HUDClass)
				{
					NewFieldPC->ClientSetHUD(GM->HUDClass);
				}
				OutControllerToRestore = NewFieldPC;
				OutCombatPCToDestroy = CombatPlayerController.Get();

				UE_LOG(LogTemp, Warning, TEXT("CombatTransitionSubsystem::OnBattleEnded : CachedFieldController 무효화됨. GameMode의 PlayerControllerClass(%s)로 새 필드 컨트롤러 스폰하여 복원."), *GetNameSafe(GM->PlayerControllerClass));
			}
			else
			{
				OutControllerToRestore = CombatPlayerController.Get();
				UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::OnBattleEnded : 새 필드 컨트롤러 스폰 실패. CombatPlayerController로 폴백."));
			}
		}
		else
		{
			OutControllerToRestore = CombatPlayerController.Get();
			UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::OnBattleEnded : CachedFieldController 무효화됨. GameMode 또는 PlayerControllerClass 없음. CombatPlayerController로 폴백."));
		}
	}
	else if (IsValid(CombatPlayerController))
	{
		OutControllerToRestore = CombatPlayerController.Get();
	}
	else if (IsValid(CachedFieldController))
	{
		OutControllerToRestore = CachedFieldController.Get();
		UE_LOG(LogTemp, Warning, TEXT("CombatTransitionSubsystem::OnBattleEnded : CombatPlayerController 유효하지 않음. CachedFieldController로 폴백."));
	}
}

void UCombatTransitionSubsystem::RestoreFieldPawn(APlayerController* ControllerToRestore, const FVector& LeaderLocation, const FRotator& LeaderRotation, bool bHasLeaderTransform)
{
	if (!IsValid(CachedFieldPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::OnBattleEnded : CachedFieldPawn이 유효하지 않음. 필드 폰 복원 불가."));
		return;
	}

	APawn* FieldPawn = CachedFieldPawn.Get();

	if (bHasLeaderTransform)
	{
		FieldPawn->SetActorLocation(LeaderLocation);
		FieldPawn->SetActorRotation(LeaderRotation);
	}

	FieldPawn->SetActorHiddenInGame(false);
	FieldPawn->SetActorEnableCollision(true);
	FieldPawn->SetActorTickEnabled(true);

	if (ControllerToRestore)
	{
		ControllerToRestore->Possess(FieldPawn);
		UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem::OnBattleEnded : PlayerPawn 빙의 복원 완료."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::OnBattleEnded : 복원할 컨트롤러 없음. PlayerPawn 가시성만 복원 (빙의 안 됨)."));
	}
}

void UCombatTransitionSubsystem::ResetTransitionState()
{
	CombatPlayerController     = nullptr;
	CachedFieldController      = nullptr;
	OriginalPlayerCharacterID  = NAME_None;
	CurrentPlayerCharacterID   = NAME_None;
	CachedFieldPawn            = nullptr;

	// 짧은 딜레이 후 전환 면역 해제 (동일 프레임 오버랩 이벤트 방지)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EncounterImmuneTimerHandle);
		World->GetTimerManager().SetTimer(EncounterImmuneTimerHandle, [this]()
		{
			bIsTransitioning = false;
		}, 0.5f, false);
	}
	else
	{
		bIsTransitioning = false;
	}
}

void UCombatTransitionSubsystem::HandleBattleEnded(const FBattleSessionSnapshot& /*Snapshot*/, EBattleEndReason Reason)
{
	OnBattleEnded(Reason);
}
