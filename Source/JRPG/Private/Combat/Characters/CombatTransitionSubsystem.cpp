#include "Combat/Characters/CombatTransitionSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Characters/CombatPlayerController.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Game/Companion/FieldCompanionSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "TimerManager.h"

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
		// 전투 진입 전 좌표 저장 (승리 시 복귀용)
		CachedPreBattleLocation = FieldPawn->GetActorLocation();
		CachedPreBattleRotation = FieldPawn->GetActorRotation();
		bHasPreBattleTransform = true;
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
			CombatPC->SetControlRotation(PC->GetControlRotation());

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
	CombatPlayerController = PC;

	// 폴백 경로에서도 필드 폰 → 전투 액터 이동 속도/입력 동기화
	SyncMovementStateToLeader(CachedFieldPawn.Get(), LeaderActor);

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem::EnterCombatMode : 전투 모드 진입 완료 (필드 PC 폴백) -> %s"), *LeaderCharacterID.ToString());
}

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
		if (ULocomotionComponent* CombatLoco = LeaderActor->FindComponentByClass<ULocomotionComponent>())
		{
			CombatLoco->SetMoveInput(FieldLoco->GetMoveInput());
			CombatLoco->SetSprint(FieldLoco->IsSprinting());
		}
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
		OldPawn->SpawnDefaultController();
	}

	CombatPlayerController->Possess(TargetActor);
	CurrentPlayerCharacterID = NewCharacterID;

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 빙의 전환 완료 → %s"), *NewCharacterID.ToString());
}

// ─── OnBattleEnded: God Method를 서브루틴으로 분해 ───

void UCombatTransitionSubsystem::OnBattleEnded(EBattleEndReason Reason)
{
	if (OriginalPlayerCharacterID.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("CombatTransitionSubsystem::OnBattleEnded : OriginalPlayerCharacterID 미설정."));
		return;
	}

	PendingEndReason = Reason;

	if (Reason == EBattleEndReason::Victory)
	{
		// 승리: 페이드 없이 즉시 전환
		PerformTransition();
		StartPostBattleRecovery();
		ResetTransitionState();
		UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 승리 전투 종료 처리 완료. 필드 모드 복원."));
	}
	else
	{
		// 패배: 2초 페이드 아웃 → 전환 → 2초 페이드 인
		StartScreenFade(0.f, 1.f, FadeDuration);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				FadeOutTimerHandle,
				this,
				&UCombatTransitionSubsystem::OnFadeOutComplete,
				FadeDuration,
				false
			);
		}
	}
}

void UCombatTransitionSubsystem::PerformTransition()
{
	// 1. 주인공 캐릭터에 빙의 복귀
	ReturnPossessionToLeader();

	// 2. 리더 트랜스폼/이동 동기화 저장
	FVector LeaderFinalLocation;
	FRotator LeaderFinalRotation;
	bool bHasLeaderTransform = false;
	SaveLeaderTransformAndSync(LeaderFinalLocation, LeaderFinalRotation, bHasLeaderTransform);

	// 3. CombatCharacterActor 스냅샷 저장 후 파괴
	UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>();
	if (SpawnSub)
	{
		TArray<ACombatCharacterActor*> CurrentActors = SpawnSub->GetSpawnedActors();
		SpawnSub->DespawnCombatActors(CurrentActors);
	}

	// 4. 승리/패배에 따른 위치 및 HP 복구 처리
	FVector RestoreLocation = LeaderFinalLocation;
	FRotator RestoreRotation = LeaderFinalRotation;
	bool bHasRestoreTransform = bHasLeaderTransform;

	if (PendingEndReason == EBattleEndReason::Defeat)
	{
		// 패배: 사망 캐릭터 부활 + HP 100% 회복
		HandleDefeatRecovery(PendingEndReason);

		// 패배: 가장 가까운 허브로 이동
		if (HubLocations.Num() > 0)
		{
			const FVector ReferenceLocation = bHasPreBattleTransform ? CachedPreBattleLocation : LeaderFinalLocation;
			RestoreLocation = FindNearestHub(ReferenceLocation);
			RestoreRotation = FRotator::ZeroRotator;
			bHasRestoreTransform = true;
		}
	}
	else if (PendingEndReason == EBattleEndReason::Victory)
	{
		// 승리: 전투 진입 전 좌표로 복귀
		if (bHasPreBattleTransform)
		{
			RestoreLocation = CachedPreBattleLocation;
			RestoreRotation = CachedPreBattleRotation;
			bHasRestoreTransform = true;
		}
	}

	// 5. 컨트롤러 스왑 복원
	APlayerController* ControllerToRestore = nullptr;
	APlayerController* CombatPCToDestroy = nullptr;
	RestoreFieldController(ControllerToRestore, CombatPCToDestroy);

	// 6. 필드 폰 복원
	RestoreFieldPawn(ControllerToRestore, RestoreLocation, RestoreRotation, bHasRestoreTransform);

	// 7. 필드 폰 복원 완료 후 전투 컨트롤러 파괴
	if (CombatPCToDestroy)
	{
		CombatPCToDestroy->Destroy();
	}

	// 8. CompanionPawn 복원
	if (UFieldCompanionSubsystem* CompanionSub = GetWorld()->GetSubsystem<UFieldCompanionSubsystem>())
	{
		CompanionSub->RestoreCompanions();
	}

	// 9. 상태 초기화
	ResetTransitionState();

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 전투 종료 처리 완료. 필드 모드 복원."));
}

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

void UCombatTransitionSubsystem::HandleDefeatRecovery(EBattleEndReason Reason)
{
	if (Reason != EBattleEndReason::Defeat) return;

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UCharacterRuntimeSubsystem* CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>())
			{
				// 사망 캐릭터 부활 + HP 100% 회복
				CharacterRuntime->RecoverPartyFromWipe(1.0f);
			}
		}
	}
}

void UCombatTransitionSubsystem::RestoreFieldController(APlayerController*& OutControllerToRestore, APlayerController*& OutCombatPCToDestroy)
{
	OutControllerToRestore = nullptr;
	OutCombatPCToDestroy = nullptr;

	if (IsValid(CachedFieldController) && IsValid(CombatPlayerController) && CombatPlayerController != CachedFieldController)
	{
		CachedFieldController->SetControlRotation(CombatPlayerController->GetControlRotation());

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
	CachedPreBattleLocation = FVector::ZeroVector;
	CachedPreBattleRotation = FRotator::ZeroRotator;
	bHasPreBattleTransform = false;
	PendingEndReason = EBattleEndReason::Aborted;
}

void UCombatTransitionSubsystem::OnFadeOutComplete()
{
	// 패배 전용 경로: 페이드 아웃 완료 후 전환 처리
	PerformTransition();

	// 페이드 인 시작 (2초)
	StartScreenFade(1.f, 0.f, FadeDuration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FadeInTimerHandle,
			this,
			&UCombatTransitionSubsystem::OnFadeInComplete,
			FadeDuration,
			false
		);
	}
}

void UCombatTransitionSubsystem::OnFadeInComplete()
{
	// 패배 전용 경로: 페이드 인 완료 후 상태 초기화
	ResetTransitionState();

	UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 패배 전투 종료 처리 완료. 필드 모드 복원."));
}

void UCombatTransitionSubsystem::StartScreenFade(float FromAlpha, float ToAlpha, float Duration)
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (PC && PC->PlayerCameraManager)
	{
		const bool bHoldWhenFinished = (ToAlpha >= 1.f);
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, Duration, FLinearColor::Black, false, bHoldWhenFinished);
	}
}

FVector UCombatTransitionSubsystem::FindNearestHub(const FVector& FromLocation) const
{
	if (HubLocations.Num() == 0) return FromLocation;

	FVector Nearest = HubLocations[0];
	float MinDistSq = FVector::DistSquared(FromLocation, Nearest);

	for (int32 i = 1; i < HubLocations.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(FromLocation, HubLocations[i]);
		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			Nearest = HubLocations[i];
		}
	}

	return Nearest;
}

void UCombatTransitionSubsystem::RegisterHubLocation(const FVector& Location)
{
	HubLocations.AddUnique(Location);
}

void UCombatTransitionSubsystem::UnregisterHubLocation(const FVector& Location)
{
	HubLocations.Remove(Location);
}

void UCombatTransitionSubsystem::StartPostBattleRecovery()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PostBattleRecoveryTimerHandle,
			this,
			&UCombatTransitionSubsystem::TickPostBattleRecovery,
			PostBattleRecoveryInterval,
			true
		);
	}
}

void UCombatTransitionSubsystem::StopPostBattleRecovery()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostBattleRecoveryTimerHandle);
	}
}

void UCombatTransitionSubsystem::TickPostBattleRecovery()
{
	UWorld* World = GetWorld();
	if (!World) { StopPostBattleRecovery(); return; }

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) { StopPostBattleRecovery(); return; }

	UCharacterRuntimeSubsystem* RuntimeSub = GI->GetSubsystem<UCharacterRuntimeSubsystem>();
	UPartySubsystem* PartySub = GI->GetSubsystem<UPartySubsystem>();
	if (!RuntimeSub || !PartySub) { StopPostBattleRecovery(); return; }

	const TArray<FName>& PartyIds = PartySub->GetPartyIds();
	bool bAllFullHP = true;

	for (const FName& Id : PartyIds)
	{
		const FCharacterResourceSnapshot* Snap = RuntimeSub->GetSnapshot(Id);
		if (!Snap || !Snap->IsValid()) continue;

		if (Snap->HP < Snap->MaxHP)
		{
			bAllFullHP = false;
			const float RecoveryAmount = Snap->MaxHP * PostBattleRecoveryRatio;
			RuntimeSub->ModifyHP(Id, RecoveryAmount);
		}
	}

	if (bAllFullHP)
	{
		StopPostBattleRecovery();
		UE_LOG(LogTemp, Log, TEXT("CombatTransitionSubsystem : 전투 후 HP 회복 완료."));
	}
}

void UCombatTransitionSubsystem::HandleBattleEnded(const FBattleSessionSnapshot& /*Snapshot*/, EBattleEndReason Reason)
{
	OnBattleEnded(Reason);
}
