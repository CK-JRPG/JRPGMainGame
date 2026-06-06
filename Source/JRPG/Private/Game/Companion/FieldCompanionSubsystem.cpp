#include "Game/Companion/FieldCompanionSubsystem.h"

#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Engine/AssetManager.h"
#include "Game/Companion/CompanionPawnController.h"
#include "Game/Companion/JRPGCompanionPawn.h"

void UFieldCompanionSubsystem::SpawnFieldCompanions(
	const FVector& LeaderLocation, 
	const FName& LeaderCharacterID,
	const TMap<FName, FCharacterSpawnEntry>& SpawnEntryMap)
{
	// FieldPawnClass 경로들을 배열로 수집(리더(플레이어) 제외)
	TArray<FSoftObjectPath> Paths;
	TArray<FName> CompanionIds;

	for (auto& Pair : SpawnEntryMap)
	{
		const FCharacterSpawnEntry& Entry = Pair.Value;
		if (Entry.FieldPawnClass.IsNull())
			continue;

		//리더(플레이어)이면 건너뜀
		if (Entry.CharacterID == LeaderCharacterID)
			continue;

		Paths.AddUnique(Entry.FieldPawnClass.ToSoftObjectPath());
		CompanionIds.Add(Entry.CharacterID);
	}

	if (Paths.IsEmpty())
		return;

	TWeakObjectPtr<UFieldCompanionSubsystem> WeakThis(this);

	// SpawnEntryMap을 참조용으로 복사 (참조로 넘어온놈이라 비동기 로드 완료 시점에 원본이 바뀔 수 있어서...)
	TMap<FName, FCharacterSpawnEntry> CapturedEntryMap = SpawnEntryMap;

	FStreamableManager& SM = UAssetManager::GetStreamableManager();

	//이전 핸들은 해제하고
	if (FieldPawnPreloadHandle.IsValid())
		FieldPawnPreloadHandle->ReleaseHandle();

	// 비동기 로드 시작 부분임
	FieldPawnPreloadHandle = SM.RequestAsyncLoad(Paths,
		[WeakThis, CompanionIds, LeaderLocation, CapturedEntryMap]()
		{
			UFieldCompanionSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->GetWorld()) return;

			UWorld* World = Self->GetWorld();
			int32 SpawnIndex = 0;

			for (const FName& ID : CompanionIds)
			{
				const FCharacterSpawnEntry* Entry = CapturedEntryMap.Find(ID);
				if (!Entry) continue;
				
				TSubclassOf<AJRPGCompanionPawn> LoadedClass = Entry->FieldPawnClass.Get();
				if (!LoadedClass) continue;

				// 플레이어 뒤쪽 좌우 배치할 위치 계산
				const float Side = (SpawnIndex % 2 == 0) ? -150.f : 150.f;
				const FVector SpawnLoc = LeaderLocation + FVector(-200.f, Side * (SpawnIndex + 1), 0.f);

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride =
					ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				AJRPGCompanionPawn* Companion = World->SpawnActor<AJRPGCompanionPawn>(
					LoadedClass, FTransform(FRotator::ZeroRotator, SpawnLoc), SpawnParams);

				if (!IsValid(Companion))
				{
					UE_LOG(LogTemp, Error, TEXT("FieldCompanionSubsystem : 컴패니언 스폰 실패 - %s"), *ID.ToString());
					continue;
				}

				Companion->UpdateCharacter(ID);
				Self->SpawnedCompanionMap.Add(ID, Companion);
				SpawnIndex++;

				UE_LOG(LogTemp, Log, TEXT("FieldCompanionSubsystem : 컴패니언 스폰 완료 - %s"), *ID.ToString());
			}
		},
		FStreamableManager::AsyncLoadHighPriority
	);
}

void UFieldCompanionSubsystem::DespawnFieldCompanions()
{
	for (auto& Pair : SpawnedCompanionMap)
		if (IsValid(Pair.Value))
			Pair.Value->Destroy();

	SpawnedCompanionMap.Empty();
}

TArray<AJRPGCompanionPawn*> UFieldCompanionSubsystem::GetSpawnedCompanions() const
{
	TArray<AJRPGCompanionPawn*> Result;
	for (auto& Pair : SpawnedCompanionMap)
		if (Pair.Value.Get()) Result.Add(Pair.Value.Get());
	return Result;
}

void UFieldCompanionSubsystem::HideCompanions()
{
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
}

FTransform UFieldCompanionSubsystem::MakeFormationTransform(const AActor* LeaderActor, int32 CompanionIndex) const
{
	if (!IsValid(LeaderActor))
	{
		return FTransform::Identity;
	}

	const int32 PartyIndex = CompanionIndex + 1;
	const FVector LeaderLocation = LeaderActor->GetActorLocation();
	const FVector LeaderForward = LeaderActor->GetActorForwardVector();
	const FRotator LeaderRotation = LeaderActor->GetActorRotation();

	float AngleOffset = (PartyIndex % 2 != 0) ? -45.0f : 45.0f;
	AngleOffset *= FMath::CeilToFloat(PartyIndex / 2.0f);

	const FVector Direction = LeaderForward.RotateAngleAxis(AngleOffset, FVector::UpVector);
	const FVector RestoreLocation = LeaderLocation - (Direction * RestoreFormationDistance);
	return FTransform(LeaderRotation, RestoreLocation);
}

bool UFieldCompanionSubsystem::IsRestoreLocationUsable(const FVector& Location, const AActor* LeaderActor) const
{
	if (Location.ContainsNaN())
	{
		return false;
	}

	if (!IsValid(LeaderActor))
	{
		return true;
	}

	const float DistanceSq = FVector::DistSquared2D(Location, LeaderActor->GetActorLocation());
	return DistanceSq <= FMath::Square(RestoreMaxDistanceFromLeader);
}

void UFieldCompanionSubsystem::MoveToFormationIfNeeded(
	AJRPGCompanionPawn* Companion,
	const AActor* LeaderActor,
	int32 CompanionIndex,
	FName ReasonTag)
{
	if (!IsValid(Companion) || !IsValid(LeaderActor))
	{
		return;
	}

	const FVector CurrentLocation = Companion->GetActorLocation();
	if (IsRestoreLocationUsable(CurrentLocation, LeaderActor))
	{
		return;
	}

	const FTransform FallbackTransform = MakeFormationTransform(LeaderActor, CompanionIndex);
	Companion->TeleportTo(FallbackTransform.GetLocation(), FallbackTransform.GetRotation().Rotator(), false, true);

	UE_LOG(LogTemp, Warning,
		TEXT("FieldCompanionSubsystem : 컴패니언 복구 위치 보정 - %s Reason=%s Old=%s New=%s Leader=%s"),
		*GetNameSafe(Companion),
		*ReasonTag.ToString(),
		*CurrentLocation.ToString(),
		*FallbackTransform.GetLocation().ToString(),
		*GetNameSafe(LeaderActor));
}

void UFieldCompanionSubsystem::RestoreCompanion(AJRPGCompanionPawn* Companion, AActor* LeaderActor, int32 CompanionIndex)
{
	if (!IsValid(Companion))
	{
		return;
	}

	MoveToFormationIfNeeded(Companion, LeaderActor, CompanionIndex, TEXT("Restore.InvalidOrFarLocation"));

	Companion->SetActorHiddenInGame(false);
	Companion->SetActorEnableCollision(true);
	Companion->SetActorTickEnabled(true);

	Companion->SpawnDefaultController();

	if (ACompanionPawnController* CompanionController = Cast<ACompanionPawnController>(Companion->GetController()))
	{
		CompanionController->SetPartyIndex(CompanionIndex + 1);
		if (IsValid(LeaderActor))
		{
			CompanionController->SetLeaderActor(LeaderActor);
		}
	}
}

void UFieldCompanionSubsystem::RestoreCompanions(AActor* LeaderActor)
{
	int32 CompanionIndex = 0;
	for (auto& Pair : SpawnedCompanionMap)
	{
		AJRPGCompanionPawn* Companion = Pair.Value.Get();
		RestoreCompanion(Companion, LeaderActor, CompanionIndex);
		++CompanionIndex;
	}
}

void UFieldCompanionSubsystem::RestoreCompanionsToSavedLocations(AActor* LeaderActor)
{
	int32 CompanionIndex = 0;
	for (auto& Pair : SpawnedCompanionMap)
	{
		AJRPGCompanionPawn* Companion = Pair.Value.Get();
		if (!IsValid(Companion))
		{
			++CompanionIndex;
			continue;
		}
		
		if (const FVector* SavedLoc = SavedCompanionLocations.Find(Pair.Key))
		{
			Companion->TeleportTo(*SavedLoc, Companion->GetActorRotation());
		}
		
		RestoreCompanion(Companion, LeaderActor, CompanionIndex);
		++CompanionIndex;
	}
}

void UFieldCompanionSubsystem::SaveCompanionLocations()
{
	SavedCompanionLocations.Empty();
	for (auto& Pair : SpawnedCompanionMap)
	{
		AJRPGCompanionPawn* Companion = Pair.Value.Get();
		if (!IsValid(Companion))
			continue;
		
		SavedCompanionLocations.Add(Pair.Key, Companion->GetActorLocation());
	}
	
	UE_LOG(LogTemp, Log, TEXT("FieldCompanionSubsystem : 컴패니언 위치 저장 완료 (%d명)"), SavedCompanionLocations.Num());
}

bool UFieldCompanionSubsystem::HasSavedCompanionLocations() const
{
	return SavedCompanionLocations.Num() > 0;
}
