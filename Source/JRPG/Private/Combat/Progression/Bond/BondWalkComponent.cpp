#include "Combat/Progression/Bond/BondWalkComponent.h"

#include "Combat/Progression/Bond/BondSubsystem.h"
#include "Combat/Progression/Bond/BondSettingsDataAsset.h"

#include "Combat/Characters/PartySubsystem.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UBondWalkComponent::UBondWalkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBondWalkComponent::BeginPlay()
{
	Super::BeginPlay();

	LastLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(Timer, this, &UBondWalkComponent::Sample, SampleIntervalSec, true);
	}
}

void UBondWalkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(Timer);
	}
	Super::EndPlay(EndPlayReason);
}


UBondSubsystem* UBondWalkComponent::GetBond() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UBondSubsystem>()
		       : nullptr;

}

const UBondSettingsDataAsset* UBondWalkComponent::GetSettings() const
{
	if (SettingsOverride) return SettingsOverride;
	if (UBondSubsystem* B = GetBond()) return B->Settings;
	return nullptr;
}


const TArray<FName>* UBondWalkComponent::GetPartyIds() const
{
	if (!GetWorld() || !GetWorld()->GetGameInstance()) return nullptr;
	if (UPartySubsystem* Party = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
		return &Party->GetPartyIds();
	return nullptr;
}

void UBondWalkComponent::Sample()
{
	if (!GetOwner()) return;
	if (UGameplayStatics::IsGamePaused(GetWorld())) return;

	const UBondSettingsDataAsset* S = GetSettings();
	UBondSubsystem* Bond = GetBond();
	const TArray<FName>* PartyIdsPtr = GetPartyIds();
	if (!S || !Bond || !PartyIdsPtr) return;

	const TArray<FName>& PartyIds = *PartyIdsPtr;
	if (PartyIds.Num() < 2 || PartyIds.Num() > 3)
		return;

	const FVector Cur = GetOwner()->GetActorLocation();
	const float Dist = FVector::Distance(Cur, LastLoc);

	const float Speed = (SampleIntervalSec > 0.f) ? (Dist / SampleIntervalSec) : 0.f;
	if (Speed < MinMoveSpeedCmPerSec)
	{
		LastLoc = Cur;
		return;
	}

	AccumTimeSec += SampleIntervalSec;
	AccumDistanceCm += Dist;

	const bool bTimeReady = (S->WalkTickSec > 0.f) && (AccumTimeSec >= S->WalkTickSec);
	const bool bDistReady = (S->WalkDistanceCm > 0.f) && (AccumDistanceCm >= S->WalkDistanceCm);

	if (bTimeReady || bDistReady)
	{
		FBondAddRequest Req;
		Req.Source = EBondSource::Walk;
		Req.Participants = PartyIds; // Trio로 넣으면 Subsystem이 페어 분배까지 처리
		Req.BaseAmount = S->WalkBPBase;
		Req.Context = "WalkTick";
		Req.SourceTag = "Bond.BP.Gained";
		Req.WorldLocation = Cur;

		Bond->AddBondPoints(Req);

		AccumTimeSec = 0.f;
		AccumDistanceCm = 0.f;
	}

	LastLoc = Cur;

}
