#include "Combat/Stats/CombatAPComponent.h"

UCombatAPComponent::UCombatAPComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatAPComponent::InitializeAP(int32 InMaxAP, bool bFillToMax)
{
	MaxAP = FMath::Max(0, InMaxAP);
	if (bFillToMax) CurrentAP = MaxAP;
	CurrentAP = FMath::Clamp(CurrentAP, 0, MaxAP);
}

bool UCombatAPComponent::Consume(int32 Amount, FName ReasonTag)
{
	if (Amount <= 0) return true;
	if (!CanConsume(Amount)) return false;

	const int32 Old = CurrentAP;
	CurrentAP = FMath::Clamp(CurrentAP - Amount, 0, MaxAP);
	OnAPChanged.Broadcast(Old, CurrentAP, ReasonTag);
	return true;
}

void UCombatAPComponent::Restore(int32 Amount, FName ReasonTag)
{
	if (Amount <= 0) return;

	const int32 Old = CurrentAP;
	CurrentAP = FMath::Clamp(CurrentAP + Amount, 0, MaxAP);
	OnAPChanged.Broadcast(Old, CurrentAP, ReasonTag);
}