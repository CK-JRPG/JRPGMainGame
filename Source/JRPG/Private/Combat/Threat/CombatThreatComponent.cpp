#include "JRPG/Public/Combat/Threat/CombatThreatComponent.h"
#include "JRPG/Public/Combat/SP/CombatSynergyPointSubsystem.h"

UCombatThreatComponent::UCombatThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
}

void UCombatThreatComponent::BeginPlay()
{
	Super::BeginPlay();
}

int32 UCombatThreatComponent::FindIndex(AActor *Source) const
{
	for (int32 i = 0; i < Table.Num(); ++i)
		if (Table[i].Source.Get() == Source) 
			return i;
	
	return INDEX_NONE;
}

void UCombatThreatComponent::AddThreat(AActor *Source, float Amount, FName)
{
	if (!Source || Amount <= 0.f) 
		return;

	const double Now = FPlatformTime::Seconds();
	const int32 Idx = FindIndex(Source);
	if (Idx == INDEX_NONE)
	{
		FThreatEntry E;
		E.Source = Source;
		E.Threat = Amount;
		E.LastUpdateReal = Now;
		Table.Add(E);
	}
	else
	{
		Table[Idx].Threat += Amount;
		Table[Idx].LastUpdateReal = Now;
	}
	
	AActor* PrevTarget = GetTopThreatSource(); //PreviousTargetActor;
	AActor* NewTarget = CurrentTarget.Get();

	if (UCombatSynergyPointSubsystem *SP = GetWorld() ? GetWorld()->GetSubsystem<UCombatSynergyPointSubsystem>() : nullptr)
	{
		const bool bBecameTopThreat = (NewTarget == Source);
		const bool bRescuedAlly = bBecameTopThreat&&PrevTarget && PrevTarget != Source;

		//Delta가 인자가 아니라서 일단 Amount로 대체했음.
		//SourceActor변수도 Source로 대체
		if (Amount > 0.f)
		{
			SP->ReportThreatOutcome(
				Source,
				GetOwner(),// 적 본체
				Amount,
				bBecameTopThreat,
				bRescuedAlly,
				false,
				ActionTag);
		}
	}
	
	OnThreatTableChanged.Broadcast(GetOwner());
}

float UCombatThreatComponent::GetThreat(AActor *Source) const
{
	const int32 Idx = FindIndex(Source);
	return (Idx == INDEX_NONE) ? 0.f : Table[Idx].Threat;
}

AActor* UCombatThreatComponent::GetTopThreatSource() const
{
	float Best = -1.f;
	AActor*BestA = nullptr;
	for (const FThreatEntry &E : Table)
	{
		if (AActor*A = E.Source.Get())
		{
			if (E.Threat > Best)
			{
				Best = E.Threat;
				BestA = A;
			}
		}
	}
	return BestA;
}

void UCombatThreatComponent::ClearAll()
{
	Table.Reset();
	OnThreatTableChanged.Broadcast(GetOwner());
}

void UCombatThreatComponent::Compact()
{
	Table.RemoveAll([](const FThreatEntry &E)
	{
		return !E.Source.IsValid() || E.Threat <= 0.f;
	});
}

void UCombatThreatComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	if (ThreatDecayPerSec <= 0.f) { Compact(); return; }

	const float Dec = FMath::Max(0.f,ThreatDecayPerSec) *DeltaTime;
	bool bChanged =false;

	for (FThreatEntry &E : Table)
	{
		if (!E.Source.IsValid()) {bChanged = true; E.Threat = 0.f; continue; }
		
		const float Old = E.Threat;
		E.Threat = FMath::Max(0.f,E.Threat - Dec);
		bChanged |= !FMath::IsNearlyEqual(Old, E.Threat,0.0001f);
	}

	Compact();
	
	if (bChanged) 
		OnThreatTableChanged.Broadcast(GetOwner());
}