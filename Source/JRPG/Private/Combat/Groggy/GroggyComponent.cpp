#include "JRPG/Public/Combat/Groggy/GroggyComponent.h"
#include "Combat/Characters/Stats/CharacterCombatStatsComponent.h"

#include "Combat/Debug/CombatDebugSubsystem.h"
#include "Combat/SP/SynergyPointSubsystem.h"
#include "Combat/Stats/CombatStatsComponent.h"

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

void UGroggyComponent::AddGroggyDamage(float Amount, AActor* SourceActor, FName ReasonTag, bool bFromTacticalReservation) 
{
	if (Amount <= 0.f) 
		return;
	
	if (bGroggy) 
		return;
	
	LastBreakSource = SourceActor;
	bLastBreakFromTacticalReservation = bFromTacticalReservation;

	Gauge = FMath::Clamp(Gauge + Amount, 0.f, MaxGauge);
	if (Gauge >= MaxGauge)
	{
		EnterGroggy();
	}
}

void UGroggyComponent::ResetGauge(FName ReasonTag)
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
		//에러 : AddModifier 없음.
		Stats->AddModifier(FCombatStatModifier::Mul(ECombatStat::Defense, FMath::Clamp(DefenseMulWhileGroggy, 0.f, 2.f),"Groggy", ModSource));
		Stats->AddModifier(FCombatStatModifier::Mul(ECombatStat::Speed,   FMath::Clamp(SpeedMulWhileGroggy, 0.f, 2.f),"Groggy", ModSource));
	}

	OnGroggyStateChanged.Broadcast(true);
	
	if (USynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<USynergyPointSubsystem>() : nullptr)
	{
		if (AActor* Source = LastBreakSource.Get())
		{
			SP->ReportBreak(Source, GetOwner(), 0.f, true, bLastBreakFromTacticalReservation, "Groggy.StunTrigger");
		}
	}
	
	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Groggy,
			"Groggy.Enter",
			FString::Printf(TEXT("Groggy entered | Duration=%.2f"), RemainingGroggy),
			GetOwner(),
			GetOwner(),
			FLinearColor(1.f,0.7f,0.2f));
	}
}

void UGroggyComponent::ExitGroggy()
{
	bGroggy = false;
	RemainingGroggy = 0.f;
	Gauge = 0.f;

	if (Stats.IsValid() && ModSource)
	{
		//에러 : RemoveModifiersBySource 없음.
		Stats->RemoveModifiersBySource(ModSource);
	}
	OnGroggyStateChanged.Broadcast(false);
	
	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Groggy,
			"Groggy.Exit",
			TEXT("Groggy exited"),
			GetOwner(),
			GetOwner(),
			FLinearColor(1.f, 0.9f, 0.4f));
	}
}

void UGroggyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!bGroggy)
		return;
	
	RemainingGroggy -= DeltaTime;
	
	if (RemainingGroggy <= 0.f)
		ExitGroggy();
}