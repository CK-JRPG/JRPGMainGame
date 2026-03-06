#include "Combat/Threat/ThreatComponent.h"

#include "Combat/Threat/ThreatConfigDataAsset.h"
#include "Combat/Stats/HPComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Combat/Infrastructure/SynergyPointSubsystem.h"

UThreatComponent::UThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UThreatComponent::BeginPlay()
{
	Super::BeginPlay();
	LastRealTime = FPlatformTime::Seconds();
	RefreshEffectiveTarget("Threat.BeginPlay");
}

void UThreatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ForcedTargetTimer);
		W->GetTimerManager().ClearTimer(LockTimer);
	}
	ThreatTable.Reset();
	Super::EndPlay(EndPlayReason);
}

const FThreatTuning& UThreatComponent::GetTuning() const
{
	static FThreatTuning Default;
	return Config ? Config->Tuning : Default;
}

float UThreatComponent::ComputeThreatMultiplier(EThreatEventKind Kind) const
{
	const FThreatTuning& T = GetTuning();
	switch (Kind)
	{
	case EThreatEventKind::DamageTaken: return T.Mult_DamageTaken;
	case EThreatEventKind::HealDone:    return T.Mult_HealDone;
	case EThreatEventKind::BuffDone:    return T.Mult_BuffDone;
	case EThreatEventKind::DirectThreat:
	default:                             return T.Mult_Direct;
	}
}

FJRPGOpResult UThreatComponent::AddThreat(AActor* Source, float Amount, FName /*ReasonTag*/)
{
	if (!Source || Amount <= 0.f)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Threat.InvalidAdd"));

	FThreatEntryRuntime& E = ThreatTable.FindOrAdd(Source);
	E.Threat = FMath::Max(0.f, E.Threat + Amount);
	E.LastUpdateRealTime = (float)FPlatformTime::Seconds();
	E.RecentHits += 1;

	OnThreatTableChanged.Broadcast(GetOwner());

	// 즉시 재선정(락/도발 고려)
	RecomputeTargetIfNeeded((float)FPlatformTime::Seconds());
	return FJRPGOpResult::Ok();
}

FJRPGOpResult UThreatComponent::ReportThreatEvent(const FThreatEvent& Ev)
{
	if (!Ev.Source || Ev.BaseAmount <= 0.f)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Threat.InvalidEvent"));

	const float Mult = ComputeThreatMultiplier(Ev.Kind);
	const float Final = Ev.BaseAmount * Mult;
	return AddThreat(Ev.Source, Final, Ev.SourceTag);
}

FJRPGOpResult UThreatComponent::ClearThreat(AActor* Source, FName /*ReasonTag*/)
{
	if (!Source)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Threat.Clear.Invalid"));

	const int32 Removed = ThreatTable.Remove(Source);
	if (Removed <= 0)
		return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Threat.Clear.NotFound"));

	OnThreatTableChanged.Broadcast(GetOwner());
	RecomputeTargetIfNeeded((float)FPlatformTime::Seconds());
	return FJRPGOpResult::Ok();
}

void UThreatComponent::ClearAllThreat(FName /*ReasonTag*/)
{
	ThreatTable.Reset();
	OnThreatTableChanged.Broadcast(GetOwner());
	SetCurrentTarget(nullptr, "Threat.ClearAll");
}

FJRPGOpResult UThreatComponent::ForceTarget(AActor* Target, float DurationSec, FName /*ReasonTag*/)
{
	if (!Target)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Threat.Force.InvalidTarget"));

	ForcedTarget = Target;
	RefreshEffectiveTarget("Threat.ForceTarget");

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ForcedTargetTimer);
		if (DurationSec > 0.f)
		{
			W->GetTimerManager().SetTimer(ForcedTargetTimer, this, &UThreatComponent::OnForcedTargetExpired, DurationSec, false);
		}
	}
	return FJRPGOpResult::Ok();
}

void UThreatComponent::ClearForcedTarget(FName /*ReasonTag*/)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ForcedTargetTimer);
	}
	ForcedTarget.Reset();
	RefreshEffectiveTarget("Threat.ClearForced");
	RecomputeTargetIfNeeded((float)FPlatformTime::Seconds());
}

FJRPGOpResult UThreatComponent::LockTarget(float DurationSec, FName /*ReasonTag*/)
{
	bTargetLocked = true;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(LockTimer);
		if (DurationSec > 0.f)
		{
			W->GetTimerManager().SetTimer(LockTimer, this, &UThreatComponent::OnLockExpired, DurationSec, false);
		}
	}
	return FJRPGOpResult::Ok();
}

void UThreatComponent::ClearLock(FName /*ReasonTag*/)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(LockTimer);
	}
	bTargetLocked = false;
	RecomputeTargetIfNeeded((float)FPlatformTime::Seconds());
}

void UThreatComponent::OnForcedTargetExpired()
{
	ClearForcedTarget("Threat.ForceExpired");
}

void UThreatComponent::OnLockExpired()
{
	ClearLock("Threat.LockExpired");
}

float UThreatComponent::GetThreat(AActor* Source) const
{
	if (!Source) return 0.f;
	if (const FThreatEntryRuntime* E = ThreatTable.Find(Source))
		return E->Threat;
	return 0.f;
}

TArray<FThreatEntryRuntime> UThreatComponent::DebugGetThreatEntries(TArray<AActor*>& OutSources) const
{
	OutSources.Reset();
	TArray<FThreatEntryRuntime> Out;
	Out.Reserve(ThreatTable.Num());

	for (const auto& It : ThreatTable)
	{
		AActor* A = It.Key.Get();
		if (!A) continue;
		OutSources.Add(A);
		Out.Add(It.Value);
	}
	return Out;
}

bool UThreatComponent::IsDeadActor(AActor* A) const
{
	if (!A) return true;
	if (const UHPComponent* HP = A->FindComponentByClass<UHPComponent>())
	{
		return HP->IsDead();
	}
	return false;
}

float UThreatComponent::DistanceMultiplier(AActor* Source) const
{
	const FThreatTuning& T = GetTuning();
	if (!T.bUseDistanceWeight) return 1.f;
	if (!GetOwner() || !Source) return 1.f;

	const FVector A = GetOwner()->GetActorLocation();
	const FVector B = Source->GetActorLocation();
	const float Dist = FVector::Dist2D(A, B);

	const float Scale = FMath::Max(1.f, T.DistanceFalloffScale);
	// 1 / (1 + d/scale)
	const float M = 1.f / (1.f + (Dist / Scale));
	return FMath::Clamp(M, T.MinDistanceMultiplier, 1.f);
}

bool UThreatComponent::HasLineOfSightTo(AActor* Source) const
{
	const FThreatTuning& T = GetTuning();
	if (!T.bUseLineOfSight) return true;
	if (!GetOwner() || !Source) return false;

	UWorld* W = GetWorld();
	if (!W) return false;

	FHitResult Hit;
	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Source->GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ThreatLOS), false);
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(Source);

	const bool bHit = W->LineTraceSingleByChannel(Hit, Start, End, T.LOSChannel, Params);
	if (!bHit) return true;

	// 무언가에 막혔으면 LOS 없음
	return false;
}

float UThreatComponent::ComputeScoreFor(AActor* Source) const
{
	const FThreatEntryRuntime* E = ThreatTable.Find(Source);
	if (!E) return 0.f;

	float Score = E->Threat;
	Score *= DistanceMultiplier(Source);

	const FThreatTuning& T = GetTuning();
	if (T.bUseLineOfSight)
	{
		if (!HasLineOfSightTo(Source))
		{
			Score *= T.NoLOSMultiplier;
		}
	}
	return Score;
}

void UThreatComponent::SetCurrentTarget(AActor* NewTarget, FName /*ReasonTag*/)
{
	AActor* Old = CurrentTarget.Get();
	if (Old == NewTarget) return;

	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(Old, NewTarget);
	
	if (UWorld*W =GetWorld())
	{
		if (USynergyPointSubsystem *SP =W->GetSubsystem<USynergyPointSubsystem>())
		{
			SP->NotifyEnemyTargetChanged(GetOwner(),Old,NewTarget);
		}
	}
	
	RefreshEffectiveTarget("Threat.TargetChanged");
}

void UThreatComponent::RefreshEffectiveTarget(FName /*ReasonTag*/)
{
	// ForcedTarget 우선
	if (ForcedTarget.IsValid() && !IsDeadActor(ForcedTarget.Get()))
	{
		EffectiveTarget = ForcedTarget;
		return;
	}
	EffectiveTarget = CurrentTarget;
}

void UThreatComponent::TickDecayAndCleanup(float RealDelta)
{
	const FThreatTuning& T = GetTuning();

	// decay + cleanup
	TArray<TWeakObjectPtr<AActor>> ToRemove;

	for (auto& It : ThreatTable)
	{
		AActor* A = It.Key.Get();
		FThreatEntryRuntime& E = It.Value;

		if (!A)
		{
			ToRemove.Add(It.Key);
			continue;
		}

		if (T.bRemoveDeadTargets && IsDeadActor(A))
		{
			ToRemove.Add(It.Key);
			continue;
		}

		switch (T.DecayMode)
		{
		case EThreatDecayMode::None:
			break;

		case EThreatDecayMode::Linear:
			E.Threat = FMath::Max(0.f, E.Threat - (T.LinearDecayPerSec * RealDelta));
			break;

		case EThreatDecayMode::Exponential:
		default:
			E.Threat = E.Threat * FMath::Exp(-T.ExpDecayK * RealDelta);
			// 너무 작아지면 제거
			if (E.Threat < 0.01f) E.Threat = 0.f;
			break;
		}

		if (E.Threat <= 0.f)
		{
			ToRemove.Add(It.Key);
		}
	}

	for (const auto& K : ToRemove)
	{
		ThreatTable.Remove(K);
	}
}

void UThreatComponent::RecomputeTargetIfNeeded(float NowRealTime)
{
	// ForcedTarget이 있으면 CurrentTarget은 참고용
	if (ForcedTarget.IsValid())
	{
		RefreshEffectiveTarget("Threat.ForcedActive");
		return;
	}

	if (bTargetLocked)
	{
		RefreshEffectiveTarget("Threat.Locked");
		return;
	}

	const FThreatTuning& T = GetTuning();
	if ((NowRealTime - LastSwitchRealTime) < T.SwitchMinIntervalSec)
	{
		// 너무 자주 바꾸지 않기
		return;
	}

	// 현재/베스트 점수 계산
	AActor* Current = CurrentTarget.Get();
	float CurrentScore = Current ? ComputeScoreFor(Current) : 0.f;

	AActor* BestActor = nullptr;
	float BestScore = -FLT_MAX;

	for (const auto& It : ThreatTable)
	{
		AActor* A = It.Key.Get();
		if (!A) continue;

		const float Score = ComputeScoreFor(A);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestActor = A;
		}
	}

	// 타겟 없음
	if (!BestActor || BestScore <= 0.f)
	{
		SetCurrentTarget(nullptr, "Threat.NoTarget");
		LastSwitchRealTime = NowRealTime;
		return;
	}

	// 전환 조건(히스테리시스)
	if (!Current)
	{
		SetCurrentTarget(BestActor, "Threat.SetInitial");
		LastSwitchRealTime = NowRealTime;
		return;
	}

	// best가 current보다 충분히 좋아야 전환
	const bool bRatioPass = (BestScore >= CurrentScore * T.SwitchRatio);
	const bool bAddPass = (BestScore >= CurrentScore + T.SwitchAdditive);

	if (BestActor != Current && (bRatioPass && bAddPass))
	{
		SetCurrentTarget(BestActor, "Threat.Switch");
		LastSwitchRealTime = NowRealTime;
	}
}

void UThreatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const double Now = FPlatformTime::Seconds();
	const float RealDelta = (float)FMath::Max(0.0, Now - LastRealTime);
	LastRealTime = Now;

	if (RealDelta <= 0.f) return;

	// decay & cleanup
	TickDecayAndCleanup(RealDelta);

	// forced target 유효성 체크
	if (ForcedTarget.IsValid() && IsDeadActor(ForcedTarget.Get()))
	{
		ClearForcedTarget("Threat.ForcedDead");
	}

	// current target 유효성 체크
	if (CurrentTarget.IsValid() && IsDeadActor(CurrentTarget.Get()))
	{
		SetCurrentTarget(nullptr, "Threat.CurrentDead");
	}

	// recompute
	RecomputeTargetIfNeeded((float)Now);

	// effective target refresh
	RefreshEffectiveTarget("Threat.Tick");
}