#include "Combat/Stats/HPComponent.h"

UHPComponent::UHPComponent()
{
	PrimaryComponentTick.bCanEverTick =false;
}

void UHPComponent::InitializeHP(float InMaxHP, bool bFillToMax)
{
	MaxHP = FMath::Max(1.f, InMaxHP);
	if (bFillToMax) CurrentHP = MaxHP;
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
}

void UHPComponent::SetMaxHP(float InMaxHP, bool bKeepRatio)
{
	const float OldMax = MaxHP;
	const float Old = CurrentHP;

	MaxHP = FMath::Max(1.f, InMaxHP);

	if (bKeepRatio&&OldMax > 0.f)
	{
		const float Ratio =Old / OldMax;
		CurrentHP = FMath::Clamp(MaxHP * Ratio, 0.f, MaxHP);
	}
	else
	{
		CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
	}

	OnHPChanged.Broadcast(Old, CurrentHP, "HP.SetMax");
}

void UHPComponent::ApplyDamage(float Amount, AActor* Instigator, FName ReasonTag)
{
	if (Amount <= 0.f || IsDead()) return;

	const float Old = CurrentHP;
	CurrentHP = FMath::Clamp(CurrentHP - Amount, 0.f, MaxHP);

	OnHPChanged.Broadcast(Old, CurrentHP, ReasonTag);

	if (CurrentHP <= 0.f)
	{
		OnDeath.Broadcast(Instigator, ReasonTag);
	}
}

void UHPComponent::Heal(float Amount, AActor* /*Instigator*/, FName ReasonTag)
{
	if (Amount <= 0.f || IsDead()) return;

	const float Old = CurrentHP;
	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, MaxHP);
	OnHPChanged.Broadcast(Old, CurrentHP, ReasonTag);
}