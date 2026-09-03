#include "Combat/SP/SPComponent.h"

USPComponent::USPComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USPComponent::InitializeSP(int32 InMaxSP, int32 InStartSP)
{
	MaxSP = FMath::Max(0, InMaxSP);
	CurrentSP = FMath::Clamp(InStartSP, 0, MaxSP);
}

void USPComponent::ImportRuntimeState(int32 InMaxSP, int32 InCurrentSP)
{
	MaxSP = FMath::Max(0, InMaxSP);
	CurrentSP = FMath::Clamp(InCurrentSP, 0, MaxSP);
}

void USPComponent::AddSP(int32 Amount, FName ReasonTag)
{
	if (Amount <= 0) return;
	const int32 Old = CurrentSP;
	CurrentSP = FMath::Clamp(CurrentSP + Amount, 0, MaxSP);
	OnSPChanged.Broadcast(Old, CurrentSP, ReasonTag);
}

bool USPComponent::ConsumeSP(int32 Amount, FName ReasonTag)
{
	if (Amount<=0) return true;
	if (CurrentSP < Amount) return false;

	const int32 Old = CurrentSP;
	CurrentSP = FMath::Clamp(CurrentSP - Amount, 0, MaxSP);
	OnSPChanged.Broadcast(Old, CurrentSP, ReasonTag);
	return true;
}
