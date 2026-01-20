#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
	Broadcast();
}

void UHealthComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.f || IsDead()) return;
	CurrentHP = FMath::Clamp(CurrentHP - Amount, 0.f, MaxHP);
	Broadcast();
}

void UHealthComponent::ApplyHeal(float Amount)
{
	if (Amount <= 0.f || IsDead()) return;
	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, MaxHP);
	Broadcast();
}

void UHealthComponent::Broadcast()
{
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);
}