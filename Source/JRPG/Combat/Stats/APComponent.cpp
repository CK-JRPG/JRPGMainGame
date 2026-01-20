#include "APComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

UAPComponent::UAPComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAPComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentAP = FMath::Clamp(CurrentAP, 0, MaxAP);
	Broadcast();
}

void UAPComponent::StartRegen()
{
	if (!GetWorld()) return;
	if (RegenTimer.IsValid()) return;
	GetWorld()->GetTimerManager().SetTimer(RegenTimer, this, &UAPComponent::TickRegen, RegenIntervalSec, true);
}

void UAPComponent::StopRegen()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(RegenTimer);
}

void UAPComponent::TickRegen()
{
	if (CurrentAP >= MaxAP) return;
	CurrentAP = FMath::Clamp(CurrentAP + 1, 0, MaxAP);
	Broadcast();
}

void UAPComponent::Spend(int32 Cost)
{
	if (Cost <= 0) return;
	CurrentAP = FMath::Clamp(CurrentAP - Cost, 0, MaxAP);
	Broadcast();
}

void UAPComponent::Broadcast()
{
	OnAPChanged.Broadcast(CurrentAP, MaxAP);
}