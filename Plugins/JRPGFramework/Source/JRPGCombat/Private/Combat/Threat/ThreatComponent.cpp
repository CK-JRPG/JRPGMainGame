#include "Combat/Threat/ThreatComponent.h"
#include "TimerManager.h"

UThreatComponent::UThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UThreatComponent::BeginPlay()
{
	Super::BeginPlay();
}

FJRPGOpResult UThreatComponent::AddThreat(AActor* Target, float Amount, FName /*ReasonTag*/)
{
	if (!Target || Amount <= 0.f)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Threat.Invalid"));

	ThreatMap.FindOrAdd(Target) += Amount;
	RecomputeTarget();
	return FJRPGOpResult::Ok();
}

void UThreatComponent::RecomputeTarget()
{
	if (LockedTarget.IsValid()) return; // lock 중이면 변경하지 않음

	AActor* Old = CurrentTarget.Get();

	float Best = -FLT_MAX;
	TWeakObjectPtr<AActor> BestT;

	for (const auto& It : ThreatMap)
	{
		if (!It.Key.IsValid()) continue;
		if (It.Value > Best)
		{
			Best = It.Value;
			BestT = It.Key;
		}
	}

	CurrentTarget = BestT;

	AActor* NewT = CurrentTarget.Get();
	if (Old != NewT)
	{
		OnTargetChanged.Broadcast(Old, NewT);
	}
}

FJRPGOpResult UThreatComponent::LockTarget(AActor* Target, float Duration, FName /*ReasonTag*/)
{
	if (!Target)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Threat.Lock.InvalidTarget"));

	LockedTarget = Target;

	// timer
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(LockTimer);
		if (Duration > 0.f)
		{
			W->GetTimerManager().SetTimer(LockTimer, this, &UThreatComponent::UnlockTarget, Duration, false);
		}
	}

	// notify change
	OnTargetChanged.Broadcast(CurrentTarget.Get(), LockedTarget.Get());
	return FJRPGOpResult::Ok();
}

void UThreatComponent::UnlockTarget(FName /*ReasonTag*/)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(LockTimer);
	}

	AActor* OldLock = LockedTarget.Get();
	LockedTarget.Reset();
	RecomputeTarget();

	// notify if changed
	OnTargetChanged.Broadcast(OldLock, GetCurrentTarget());
}