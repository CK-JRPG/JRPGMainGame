#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Characters/Stats/CombatStatsComponent.h"

UGroggyComponent::UGroggyComponent()
{
	PrimaryComponentTick.bCanEverTick =true;
	PrimaryComponentTick.TickInterval =0.05f;
}

void UGroggyComponent::BeginPlay()
{
	Super::BeginPlay();
	Stats = GetOwner() ? GetOwner()->FindComponentByClass<UCombatStatsComponent>() : nullptr;
}

void UGroggyComponent::AddGroggyDamage(float Amount, AActor*, FName)
{
	if (Amount <= 0.f) 
		return;
	
	if (bGroggy) 
		return;

	Gauge = FMath::Clamp(Gauge + Amount, 0.f, MaxGauge);
	if (Gauge >= MaxGauge)
	{
		EnterGroggy();
	}
}

void UGroggyComponent::ResetGauge(FName)
{
	Gauge = 0.f;
	
	if (bGroggy)
		ExitGroggy();
}

void UGroggyComponent::EnterGroggy()
{
	bGroggy = true;
	RemainingGroggy = FMath::Max(0.1f, GroggyDurationSec);

	if (!ModSource)
		ModSource = NewObject<UGroggyModSourceObject>(this);

	if (Stats.IsValid())
	{
		Stats->AddModifier(FCombatStatModifier::Mul(ECombatStat::Defense, FMath::Clamp(DefenseMulWhileGroggy, 0.f, 2.f),"Groggy", ModSource));
		Stats->AddModifier(FCombatStatModifier::Mul(ECombatStat::Speed,   FMath::Clamp(SpeedMulWhileGroggy, 0.f, 2.f),"Groggy", ModSource));
	}

	OnGroggyStateChanged.Broadcast(true);
}

void UGroggyComponent::ExitGroggy()
{
	bGroggy = false;
	RemainingGroggy = 0.f;
	Gauge = 0.f;

	if (Stats.IsValid() && ModSource)
	{
		Stats->RemoveModifiersBySource(ModSource);
	}
	OnGroggyStateChanged.Broadcast(false);
}

void UGroggyComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	if (!bGroggy)
		return;
	
	RemainingGroggy -= DeltaTime;
	
	if (RemainingGroggy <= 0.f)
		ExitGroggy();
}