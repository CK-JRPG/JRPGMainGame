#include "ThreatComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

UThreatComponent::UThreatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UThreatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UThreatComponent::InitThreat(const TArray<AActor*>& PartyMembers)
{
    ThreatTable.Reset();
    for (AActor* A : PartyMembers)
        if (A) ThreatTable.Add(A, 0.f);

    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UThreatComponent::UpdateTarget, TargetUpdateIntervalSec, true);
}

void UThreatComponent::AddThreat(AActor* Target, float Amount)
{
    if (!Target || Amount <= 0.f) return;
    ThreatTable.FindOrAdd(Target) += Amount;
}

void UThreatComponent::ReduceThreat(AActor* Target, float Amount)
{
    if (!Target || Amount <= 0.f) return;
    float& V = ThreatTable.FindOrAdd(Target);
    V = FMath::Max(0.f, V - Amount);
}

float UThreatComponent::GetThreatOf(AActor* Target) const
{
    if (!Target) return 0.f;
    if (const float* V = ThreatTable.Find(Target)) return *V;
    return 0.f;
}

bool UThreatComponent::IsForceActive() const
{
    if (!GetWorld()) return false;
    return ForcedTarget.IsValid() && (GetWorld()->GetRealTimeSeconds() < ForcedTargetEndRealTime);
}

void UThreatComponent::ForceTarget(AActor* NewTarget, float DurationSec)
{
    if (!NewTarget || !GetWorld()) return;

    ForcedTarget = NewTarget;
    ForcedTargetEndRealTime = GetWorld()->GetRealTimeSeconds() + FMath::Max(0.f, DurationSec);

    AActor* Old = CurrentTarget.Get();
    CurrentTarget = NewTarget;
    if (Old != NewTarget)
        OnTargetChanged.Broadcast(Old, NewTarget);
}

void UThreatComponent::UpdateTarget()
{
    if (IsForceActive())
    {
        AActor* Forced = ForcedTarget.Get();
        if (Forced && Forced != CurrentTarget.Get())
        {
            AActor* Old = CurrentTarget.Get();
            CurrentTarget = Forced;
            OnTargetChanged.Broadcast(Old, Forced);
        }
        return;
    }

    AActor* Old = CurrentTarget.Get();
    AActor* BestTarget = Old;
    float BestThreat = Old ? GetThreatOf(Old) : -1.f;

    for (const auto& It : ThreatTable)
    {
        AActor* Candidate = It.Key.Get();
        if (!Candidate) continue;

        if (It.Value > BestThreat)
        {
            BestThreat = It.Value;
            BestTarget = Candidate;
        }
    }

    if (Old && BestTarget && BestTarget != Old)
    {
        const float OldThreat = GetThreatOf(Old);
        if (BestThreat < OldThreat * TargetSwitchThresholdRatio)
            BestTarget = Old;
    }

    if (BestTarget != Old)
    {
        CurrentTarget = BestTarget;
        OnTargetChanged.Broadcast(Old, BestTarget);
    }
}