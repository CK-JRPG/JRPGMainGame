#include "Combat/Stats/APComponent.h"

UAPComponent::UAPComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAPComponent::InitializeAP(int32 InMaxAP, bool bFillToMax)
{
	MaxAP = FMath::Max(0, InMaxAP);
	if (bFillToMax) CurrentAP = MaxAP;
	CurrentAP = FMath::Clamp(CurrentAP, 0, MaxAP);
}

bool UAPComponent::Consume(int32 Amount, FName ReasonTag)
{
	if (Amount <= 0) return true;
	if (!CanConsume(Amount)) return false;

	const int32 Old = CurrentAP;
	CurrentAP = FMath::Clamp(CurrentAP - Amount, 0, MaxAP);
	OnAPChanged.Broadcast(Old, CurrentAP, ReasonTag);
	return true;
}

void UAPComponent::Restore(int32 Amount, FName ReasonTag)
{
	if (Amount <= 0) return;

	const int32 Old = CurrentAP;
	CurrentAP = FMath::Clamp(CurrentAP + Amount, 0, MaxAP);
	OnAPChanged.Broadcast(Old, CurrentAP, ReasonTag);
}