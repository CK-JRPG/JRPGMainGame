#include "Combat/Characters/PartyActorSpawnSubsystem.h"

#include "Combat/Movement/LocomotionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h" 
#include "Combat/Characters/CombatPlayerController.h"
#include "Engine/AssetManager.h"
#include "Game/Companion/JRPGCompanionPawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"

void UPartyActorSpawnSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	SpawnEntryMap.Empty();
	SpawnedActorMap.Empty();

	if (UBattleSessionSubsystem* BattleSub = InWorld.GetSubsystem<UBattleSessionSubsystem>())
	{
		BattleSub->OnBattleEnded.AddUObject(this, &UPartyActorSpawnSubsystem::HandleBattleEnded);
	}

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 초기화 완료."));
}

void UPartyActorSpawnSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	TArray<ACombatCharacterActor*> Remaining;
	for (auto& Pair : SpawnedActorMap)
		if (Pair.Value.Get()) Remaining.Add(Pair.Value.Get());
	DespawnCombatActors(Remaining);
}

void UPartyActorSpawnSubsystem::RegisterSpawnEntry(const FCharacterSpawnEntry& Entry)
{
	if (Entry.CharacterID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::RegisterSpawnEntry : 실패 - CharacterID가 None임."));
		return;
	}

	SpawnEntryMap.Add(Entry.CharacterID, Entry);
	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::RegisterSpawnEntry : 등록 - %s"), *Entry.CharacterID.ToString());
	// 박용석 : 스냅샷 초기화는 JRPGPlayerController::InitializeCombatBridge에서 하니까 기억하자

}

void UPartyActorSpawnSubsystem::RegisterSpawnEntries(const TArray<FCharacterSpawnEntry>& Entries)
{
	for (const FCharacterSpawnEntry& Entry : Entries)
		RegisterSpawnEntry(Entry);
}

void UPartyActorSpawnSubsystem::PreloadAssets(const TArray<FName>& PartyIds)
{
	TArray<FSoftObjectPath> Paths = CollectSoftPaths(PartyIds);
	if (Paths.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::PreloadAssets : 실패 - 로드할 에셋이 없음."));
		return;
	}

	if (PreloadHandle.IsValid())
		PreloadHandle->ReleaseHandle();
	
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	PreloadHandle = StreamableManager.RequestAsyncLoad(Paths, []()
	{
		UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 에셋 사전 로드 완료."));
	}, FStreamableManager::AsyncLoadHighPriority);
}

void UPartyActorSpawnSubsystem::SpawnFieldCompanions(const FVector& LeaderLocation, const FName& LeaderCharacterID)
{
	// FieldPawnClass 소프트 경로 수집
    TArray<FSoftObjectPath> Paths;
    TArray<FName> CompanionIds;

    for (auto& Pair : SpawnEntryMap)
    {
        const FCharacterSpawnEntry& Entry = Pair.Value;
        if (Entry.FieldPawnClass.IsNull()) 
        	continue;  // 미설정은 건너뜀
    	
        if (Entry.CharacterID == LeaderCharacterID) 
        	continue;  // 리더는 JRPGPlayerPawn이므로 건너뜁니당

        Paths.AddUnique(Entry.FieldPawnClass.ToSoftObjectPath());
        CompanionIds.Add(Entry.CharacterID);
    }

    if (Paths.IsEmpty()) 
    	return;

    TWeakObjectPtr<UPartyActorSpawnSubsystem> WeakThis(this);

    FStreamableManager& SM = UAssetManager::GetStreamableManager();

    if (FieldPawnPreloadHandle.IsValid())
        FieldPawnPreloadHandle->ReleaseHandle();

    FieldPawnPreloadHandle = SM.RequestAsyncLoad(Paths,
        [WeakThis, CompanionIds, LeaderLocation]()
        {
            UPartyActorSpawnSubsystem* Self = WeakThis.Get();
            if (!Self || !Self->GetWorld()) return;

            UWorld* World = Self->GetWorld();
            int32 SpawnIndex = 0;

            for (const FName& ID : CompanionIds)
            {
                const FCharacterSpawnEntry* Entry = Self->SpawnEntryMap.Find(ID);
                if (!Entry) continue;

                TSubclassOf<AJRPGCompanionPawn> LoadedClass = Entry->FieldPawnClass.Get();
                if (!LoadedClass) continue;

                // 플레이어 뒤쪽 좌우 배치
                const float Side = (SpawnIndex % 2 == 0) ? -150.f : 150.f;
                const FVector SpawnLoc = LeaderLocation + FVector(-200.f, Side * (SpawnIndex + 1), 0.f);

                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride =
                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                AJRPGCompanionPawn* Companion = World->SpawnActor<AJRPGCompanionPawn>(
                    LoadedClass, FTransform(FRotator::ZeroRotator, SpawnLoc), SpawnParams);

                if (!IsValid(Companion))
                {
                    UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 컴패니언 스폰 실패 - %s"), *ID.ToString());
                    continue;
                }

                Companion->UpdateCharacter(ID);
                Self->SpawnedCompanionMap.Add(ID, Companion);
                SpawnIndex++;

                UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 컴패니언 스폰 완료 - %s"), *ID.ToString());
            }
        },
        FStreamableManager::AsyncLoadHighPriority
    );
}

void UPartyActorSpawnSubsystem::DespawnFieldCompanions()
{
	for (auto& Pair : SpawnedCompanionMap)
		if (IsValid(Pair.Value))
			Pair.Value->Destroy();

	SpawnedCompanionMap.Empty();
}

TArray<AJRPGCompanionPawn*> UPartyActorSpawnSubsystem::GetSpawnedCompanions() const
{
	TArray<AJRPGCompanionPawn*> Result;
	for (auto& Pair : SpawnedCompanionMap)
		if (Pair.Value.Get()) Result.Add(Pair.Value.Get());
	return Result;
}

void UPartyActorSpawnSubsystem::AsyncSpawnCombatActorsAtFieldPositions(
	const TArray<FName>& PartyIds,
	const TMap<FName, FTransform>& FieldTransforms,
	TFunction<void(TArray<ACombatCharacterActor*>)> OnComplete)
{
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::AsyncSpawnCombatActorsAtFieldPositions : 실패 - PartyIds가 비어 있음."));
		if (OnComplete) OnComplete({});
		return;
	}

	TWeakObjectPtr<UPartyActorSpawnSubsystem> WeakThis(this);

	auto DoSpawn = [WeakThis, PartyIds, FieldTransforms, OnComplete]()
	{
		UPartyActorSpawnSubsystem* Self = WeakThis.Get();
		if (!Self)
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 에러 - 스폰 완료 전 Subsystem 소멸."));
			return;
		}

		UWorld* World = Self->GetWorld();
		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 에러 - 스폰 완료 시 World 없음."));
			return;
		}

		UCharacterRuntimeSubsystem* CharacterRuntime = nullptr;
		if (UGameInstance* GI = World->GetGameInstance())
			CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>();

		TArray<ACombatCharacterActor*> SpawnedActors;
		SpawnedActors.Reserve(PartyIds.Num());

		for (const FName& ID : PartyIds)
		{
			const FCharacterSpawnEntry* Entry = Self->SpawnEntryMap.Find(ID);
			if (!Entry)
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 에러 - SpawnEntry 없음. ID: %s"), *ID.ToString());
				continue;
			}

			TSubclassOf<ACombatCharacterActor> LoadedClass =
				TSoftClassPtr<ACombatCharacterActor>(Entry->ActorClass.ToSoftObjectPath()).Get();
			if (!LoadedClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 에러 - 클래스 로드 실패. ID: %s"), *ID.ToString());
				continue;
			}

			// 필드 폰 위치가 있으면 그 위치에, 없으면 폴백
			FTransform ActorTransform;
			if (const FTransform* FieldTransform = FieldTransforms.Find(ID))
			{
				ActorTransform = *FieldTransform;
			}
			else
			{
				ActorTransform = FTransform::Identity;
				ActorTransform.AddToTranslation(Entry->SpawnOffset);
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 필드 트랜스폼 없음, 폴백 사용 - %s"), *ID.ToString());
			}

			ACombatCharacterActor* SpawnedActor = Self->SpawnSingleActor(LoadedClass, ActorTransform);
			if (!IsValid(SpawnedActor))
			{
				UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 스폰 실패 - %s"), *ID.ToString());
				continue;
			}

			Self->SpawnedActorMap.Add(ID, SpawnedActor);
			SpawnedActors.Add(SpawnedActor);

			if (CharacterRuntime)
				CharacterRuntime->RestoreSnapshot(ID, SpawnedActor);

			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 필드 위치 기반 스폰 성공 - %s"), *ID.ToString());
		}

		if (OnComplete) OnComplete(SpawnedActors);
	};

	if (PreloadHandle.IsValid() && PreloadHandle->HasLoadCompleted())
	{
		DoSpawn();
		return;
	}

	TArray<FSoftObjectPath> Paths = CollectSoftPaths(PartyIds);
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	if (PreloadHandle.IsValid())
		PreloadHandle->ReleaseHandle();

	PreloadHandle = StreamableManager.RequestAsyncLoad(
		Paths,
		[DoSpawn]() { DoSpawn(); },
		FStreamableManager::AsyncLoadHighPriority
	);
}


void UPartyActorSpawnSubsystem::DespawnCombatActors(const TArray<ACombatCharacterActor*>& Actors)
{
	UCharacterRuntimeSubsystem* CharacterRuntime = nullptr;
	if (UWorld* World = GetWorld())
		if (UGameInstance* GI = World->GetGameInstance())
			CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>();

	for (ACombatCharacterActor* Actor : Actors)
	{
		if (!IsValid(Actor)) continue;

		// AI 컨트롤러가 있으면 먼저 제거
		if (AController* C = Actor->GetController())
		{
			if (!C->IsPlayerController())
			{
				C->UnPossess();
				C->Destroy();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("DespawnCombatActors : PlayerController 빙의 중 Destroy 시도 - %s"), *Actor->GetName());
				C->UnPossess();
			}
		}
		
		// SpawnedActorMap에서 제거
		for (auto It = SpawnedActorMap.CreateIterator(); It; ++It)
		{
			if (It.Value() == Actor)
			{
				It.RemoveCurrent();
				break;
			}
		}

		// DCombatCharacterActor 파괴되는거 직전에 HP/AP/SP 저장
		if (CharacterRuntime)
		{
			const FName CharID = Actor->GetCombatantId();
			if (!CharID.IsNone())
				CharacterRuntime->SaveSnapshot(CharID, Actor);
		}

		Actor->Destroy();
	}

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : %d개 액터 디스폰 완료."), Actors.Num());
}

void UPartyActorSpawnSubsystem::SetOriginalPlayerCharacterID(const FName& CharacterID)
{
	OriginalPlayerCharacterID = CharacterID;
	CurrentPlayerCharacterID  = CharacterID;

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 원본 플레이어 캐릭터 설정 - %s"), *CharacterID.ToString());
}

void UPartyActorSpawnSubsystem::SetCombatPlayerController(APlayerController* InController)
{
	CombatPlayerController = InController;
}

void UPartyActorSpawnSubsystem::SetCombatControllerClass(TSubclassOf<APlayerController> InClass)
{
	CombatControllerClass = InClass;
	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : CombatControllerClass 설정 → %s"), *GetNameSafe(InClass));
}

void UPartyActorSpawnSubsystem::EnterCombatMode(APlayerController* PC, const FName& LeaderCharacterID)
{
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem::EnterCombatMode : PlayerController가 없음."));
		return;
	}

	ACombatCharacterActor* LeaderActor = FindActorByCharacterID(LeaderCharacterID);
	if (!LeaderActor)
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem::EnterCombatMode : 리더 CombatCharacterActor를 찾을 수 없음 - %s"), *LeaderCharacterID.ToString());
		return;
	}

	// 상태 저장
	SetCombatPlayerController(PC);
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

	// CompanionPawn도 숨기고서 AI 컨트롤러 제거 <- 아직 전투 제거 후 재할당 안만듬
	for (auto& Pair : SpawnedCompanionMap)
	{
		AJRPGCompanionPawn* Companion = Pair.Value.Get();
		if (!IsValid(Companion)) continue;

		Companion->SetActorHiddenInGame(true);
		Companion->SetActorEnableCollision(false);
		Companion->SetActorTickEnabled(false);

		if (AController* AI = Companion->GetController())
		{
			AI->UnPossess();
			AI->Destroy();
		}
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
			
			// 필드 컨트롤러 -> 전투 컨트롤러로 LocalPlayer 스왑
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
			SetCombatPlayerController(CombatPC);

			//필드 캐릭터 -> 전투 액터로 전환시 이동, 입력속도 동기화
			SyncMovementStateToLeader(CachedFieldPawn.Get(), LeaderActor);
			
			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::EnterCombatMode : CombatPlayerController 스왑 완료 → %s"), *LeaderCharacterID.ToString());
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::EnterCombatMode : CombatPlayerController 스폰 실패. 필드 PC로 폴백."));
		}
	}

	// 폴백 : CombatPlayerController 스폰 실패 시 기존 필드 PC 사용
	PC->Possess(LeaderActor);
	SetCombatPlayerController(PC);
	
	//폴백시에도 이동속도 동기화
	SyncMovementStateToLeader(CachedFieldPawn.Get(), LeaderActor);

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::EnterCombatMode : 전투 모드 진입 완료 (필드 PC 폴백) -> %s"), *LeaderCharacterID.ToString());
}

void UPartyActorSpawnSubsystem::SyncMovementStateToLeader(APawn* FieldPawn, ACombatCharacterActor* LeaderActor)
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

void UPartyActorSpawnSubsystem::SyncMovementStateToFieldPawn(ACombatCharacterActor* LeaderActor, APawn* FieldPawn)
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

void UPartyActorSpawnSubsystem::OnPartyMemberChanged(const FName& NewCharacterID)
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	if (!BattleSession || !BattleSession->IsBattleActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem : 필드 상태에서 조작 캐릭터 전환 불가."));
		return;
	}

	if (CurrentPlayerCharacterID == NewCharacterID) return;

	ACombatCharacterActor* TargetActor = FindActorByCharacterID(NewCharacterID);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : 전환 대상 Actor 없음 - %s"), *NewCharacterID.ToString());
		return;
	}

	if (!CombatPlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem : CombatPlayerController 미등록."));
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

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 빙의 전환 완료 → %s"), *NewCharacterID.ToString());
}

void UPartyActorSpawnSubsystem::OnBattleEnded(EBattleEndReason Reason)
{
	if (OriginalPlayerCharacterID.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : OriginalPlayerCharacterID 미설정."));
		return;
	}

	// 다른 캐릭터에 빙의 중이면 주인공한테 카메라 복귀 부분
	if (CurrentPlayerCharacterID != OriginalPlayerCharacterID && CombatPlayerController)
	{
		ACombatCharacterActor* LeaderActor = FindActorByCharacterID(OriginalPlayerCharacterID);
		if (LeaderActor)
		{
			// 현재 빙의 중인 캐릭터 해제 → AI 복원
			APawn* OldPawn = CombatPlayerController->GetPawn();
			CombatPlayerController->UnPossess();
			if (IsValid(OldPawn))
			{
				OldPawn->SpawnDefaultController();
			}

			// 리더의 기존 AI 제거 후 플레이어 빙의 → OnPossess에서 카메라 타겟 자동 갱신
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

			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : 주인공 캐릭터에 카메라 복귀 완료 -> %s"), *OriginalPlayerCharacterID.ToString());
		}
	}
	
	// CombatCharacterActor에서 빙의 해제
	if (CombatPlayerController)
	{
		CombatPlayerController->UnPossess();
	}

	// 리더 CombatCharacterActor의 최종 위치/회전 저장 (필드 폰 복원 시 동기화용)
	FVector LeaderFinalLocation = FVector::ZeroVector;
	FRotator LeaderFinalRotation = FRotator::ZeroRotator;
	bool bHasLeaderTransform = false;
	if (ACombatCharacterActor* LeaderForRestore = FindActorByCharacterID(OriginalPlayerCharacterID))
	{
		LeaderFinalLocation = LeaderForRestore->GetActorLocation();
		LeaderFinalRotation = LeaderForRestore->GetActorRotation();
		bHasLeaderTransform = true;
		
		// 전투 액터 -> 필드 폰 이동 속도/입력 동기화 (파괴 전에 수행)
		if (IsValid(CachedFieldPawn))
		{
			SyncMovementStateToFieldPawn(LeaderForRestore, CachedFieldPawn.Get());
		}
	}
	
	// CombatCharacterActor 스냅샷 저장 후 파괴
	TArray<ACombatCharacterActor*> CurrentActors;
	for (auto& Pair : SpawnedActorMap)
		if (Pair.Value.Get()) CurrentActors.Add(Pair.Value.Get());
	DespawnCombatActors(CurrentActors);
	if (Reason == EBattleEndReason::Defeat)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				if (UCharacterRuntimeSubsystem* CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>())
				{
					CharacterRuntime->RecoverPartyFromWipe();
				}
			}
		}
	}
	
	// 전투 전용 컨트롤러 -> 필드 컨트롤러로 스왑 복원
	APlayerController* ControllerToRestoreWith = nullptr;
	APlayerController* CombatPCToDestroy = nullptr;

	if (IsValid(CachedFieldController) && IsValid(CombatPlayerController) && CombatPlayerController != CachedFieldController)
	{
		CachedFieldController->SetControlRotation(CombatPlayerController->GetControlRotation());
		
		// CombatPlayerController를 사용했으므로 스왑 복원
		if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
		{
			GM->SwapPlayerControllers(CombatPlayerController.Get(), CachedFieldController.Get());
			
			// 원래 컨트롤러로 돌아온 직후 탐험용 HUD 생성 명령
		  	if (GM->HUDClass)
		  	{
		  		CachedFieldController->ClientSetHUD(GM->HUDClass);
		  	}
		}
		ControllerToRestoreWith = CachedFieldController.Get();

		// 전투 컨트롤러 파괴는 필드 폰 복원 후에 수행 (Destroy 부작용 방지)
		CombatPCToDestroy = CombatPlayerController.Get();
		UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : CombatPlayerController -> 필드 컨트롤러 스왑 복원."));
	}
	else if (IsValid(CombatPlayerController))
	{
		// 폴백 : 같은 PC를 사용한 경우
		ControllerToRestoreWith = CombatPlayerController.Get();
	}
	else if (IsValid(CachedFieldController))
	{
		// 폴백 : CombatPlayerController 없을 때 CachedFieldController 사용
		ControllerToRestoreWith = CachedFieldController.Get();
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : CombatPlayerController 유효하지 않음. CachedFieldController로 폴백."));
	}
	
	// JRPGPlayerPawn 필드 복원 (항상 수행)
	if (IsValid(CachedFieldPawn))
	{
		APawn* FieldPawn = CachedFieldPawn.Get();
		
		// CombatCharacterActor의 최종 위치와 회전을 필드쪽 캐릭터에 동기화
		if (bHasLeaderTransform)
		{
			FieldPawn->SetActorLocation(LeaderFinalLocation);
			FieldPawn->SetActorRotation(LeaderFinalRotation);
		}
		
		FieldPawn->SetActorHiddenInGame(false);
		FieldPawn->SetActorEnableCollision(true);
		FieldPawn->SetActorTickEnabled(true);

		if (ControllerToRestoreWith)
		{
			ControllerToRestoreWith->Possess(FieldPawn);
			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : PlayerPawn 빙의 복원 완료."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : 복원할 컨트롤러 없음. PlayerPawn 가시성만 복원 (빙의 안 됨)."));
		}		
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : CachedFieldPawn이 유효하지 않음. 필드 폰 복원 불가."));
	}

	// 필드 폰 복원 완료 후 전투 컨트롤러 파괴
	if (CombatPCToDestroy)
	{
		CombatPCToDestroy->Destroy();
	}
	
	// CompanionPawn 복원시킴
	for (auto& Pair : SpawnedCompanionMap)
	{
		AJRPGCompanionPawn* Companion = Pair.Value.Get();
		if (!IsValid(Companion)) continue;

		Companion->SetActorHiddenInGame(false);
		Companion->SetActorEnableCollision(true);
		Companion->SetActorTickEnabled(true);

		//여기 BP AI컨트롤러 설정 하는거 다시해야함(빙의 현재 안되는 중.)
		Companion->SpawnDefaultController();
	}

	// 나머지 상태 초기화
	CombatPlayerController     = nullptr;
	CachedFieldController      = nullptr;
	OriginalPlayerCharacterID  = NAME_None;
	CurrentPlayerCharacterID   = NAME_None;
	CachedFieldPawn            = nullptr;

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 전투 종료 처리 완료. 필드 모드 복원."));
}

void UPartyActorSpawnSubsystem::HandleBattleEnded(const FBattleSessionSnapshot& /*Snapshot*/,EBattleEndReason Reason)
{
	OnBattleEnded(Reason);
}

TArray<ACombatCharacterActor*> UPartyActorSpawnSubsystem::GetSpawnedActors() const
{
	TArray<ACombatCharacterActor*> Result;
	for (auto& Pair : SpawnedActorMap)
		if (Pair.Value.Get()) Result.Add(Pair.Value.Get());
	return Result;
}

ACombatCharacterActor* UPartyActorSpawnSubsystem::FindActorByCharacterID(const FName& CharacterID) const
{
	if (const TObjectPtr<ACombatCharacterActor>* Found = SpawnedActorMap.Find(CharacterID))
		return Found->Get();
	return nullptr;
}

ACombatCharacterActor* UPartyActorSpawnSubsystem::SpawnSingleActor(
	TSubclassOf<ACombatCharacterActor> ActorClass, const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	
	if (!World || !ActorClass)
		return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return World->SpawnActor<ACombatCharacterActor>(ActorClass, SpawnTransform, SpawnParams);
}

TArray<FSoftObjectPath> UPartyActorSpawnSubsystem::CollectSoftPaths(const TArray<FName>& PartyIds) const
{
	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(PartyIds.Num());

	for (const FName& ID : PartyIds)
	{
		if (const FCharacterSpawnEntry* Entry = SpawnEntryMap.Find(ID))
		{
			if (!Entry->ActorClass.IsNull())
				Paths.AddUnique(Entry->ActorClass.ToSoftObjectPath());
			else
				UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::CollectSoftPaths : %s의 ActorClass가 null."), *ID.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::CollectSoftPaths : 매핑 없음 - %s"), *ID.ToString());
		}
	}

	return Paths;
}