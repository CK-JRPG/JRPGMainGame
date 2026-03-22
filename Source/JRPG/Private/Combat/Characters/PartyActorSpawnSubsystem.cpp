#include "Combat/Characters/PartyActorSpawnSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"  // 추가
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Engine/AssetManager.h"
#include "Game/Companion/JRPGCompanionPawn.h"

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

void UPartyActorSpawnSubsystem::AsyncSpawnCombatActors(const TArray<FName>& PartyIds, const FTransform& SpawnOrigin,
                                                       TFunction<void(TArray<ACombatCharacterActor*>)> OnComplete)
{
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PartyActorSpawnSubsystem::AsyncSpawnCombatActors : 실패 - PartyIds가 비어 있음."));
		if (OnComplete) OnComplete({});
			return;
	}

	TWeakObjectPtr<UPartyActorSpawnSubsystem> WeakThis(this);

	//실제 스폰 로직들
	auto DoSpawn = [WeakThis, PartyIds, SpawnOrigin, OnComplete]()
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

			FTransform ActorTransform = SpawnOrigin;
			ActorTransform.AddToTranslation(Entry->SpawnOffset);

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
 
			UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 스폰 성공 - %s"), *ID.ToString());
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

	//리더인  CombatCharacterActor에 빙의시킴
	PC->Possess(LeaderActor);

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::EnterCombatMode : 전투 모드 진입 완료 → %s"), *LeaderCharacterID.ToString());
}

void UPartyActorSpawnSubsystem::OnPartyMemberChanged(const FName& NewCharacterID)
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	if (!BattleSession->IsBattleActive())
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

	CombatPlayerController->UnPossess();
	CombatPlayerController->Possess(TargetActor);
	CurrentPlayerCharacterID = NewCharacterID;

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 빙의 전환 완료 → %s"), *NewCharacterID.ToString());
}

void UPartyActorSpawnSubsystem::OnBattleEnded()
{
	if (OriginalPlayerCharacterID.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : OriginalPlayerCharacterID 미설정."));
		return;
	}

	// CombatCharacterActor에서 빙의 해제
	if (CombatPlayerController)
	{
		CombatPlayerController->UnPossess();
	}

	// CombatCharacterActor 스냅샷 저장 후 파괴
	TArray<ACombatCharacterActor*> CurrentActors;
	for (auto& Pair : SpawnedActorMap)
		if (Pair.Value.Get()) CurrentActors.Add(Pair.Value.Get());
	DespawnCombatActors(CurrentActors);

	// JRPGPlayerPawn으로 다시 복원 시키고 JRPGPlayerPawn으로 빙의
	if (IsValid(CachedFieldPawn) && CombatPlayerController)
	{
		APawn* FieldPawn = CachedFieldPawn.Get();
		FieldPawn->SetActorHiddenInGame(false);
		FieldPawn->SetActorEnableCollision(true);
		FieldPawn->SetActorTickEnabled(true);

		CombatPlayerController->Possess(FieldPawn);

		UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem::OnBattleEnded : PlayerPawn 빙의 복원 완료."));
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
	OriginalPlayerCharacterID  = NAME_None;
	CurrentPlayerCharacterID   = NAME_None;
	CachedFieldPawn            = nullptr;

	UE_LOG(LogTemp, Log, TEXT("PartyActorSpawnSubsystem : 전투 종료 처리 완료. 필드 모드 복원."));
}

void UPartyActorSpawnSubsystem::HandleBattleEnded(const FBattleSessionSnapshot& /*Snapshot*/, EBattleEndReason /*Reason*/)
{
	OnBattleEnded();
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