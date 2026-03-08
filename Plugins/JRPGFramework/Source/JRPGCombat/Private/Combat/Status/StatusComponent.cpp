#include "Combat/Status/StatusComponent.h"

#include "Combat/Infrastructure/BattleSessionSubsystem.h"
#include "Combat/Status/StatusDataAsset.h"

UStatusComponent::UStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	LastRealTime = FPlatformTime::Seconds();
	RecomputeCC();
}

bool UStatusComponent::HasStatus(FName StatusId) const
{
	return Active.Contains(StatusId);
}

FJRPGOpResult UStatusComponent::ApplyStatus(const FStatusSpec& Spec)
{
	if (UBattleSessionSubsystem* Session = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		if (Session->ShouldGateEnemyToAlly(Spec.Instigator.Get(),GetOwner()))
		{
			return; // 적→아군 디버프/상태이상 적용 금지
		}
	}
	
	if (Spec.StatusId.IsNone())
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Status.InvalidId"));

	const FStatusDef* Def = StatusDB ? StatusDB->FindDef(Spec.StatusId) : nullptr;
	if (!Def)
		return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Status.DefNotFound"));

	const float UseDuration = (Spec.Duration > 0.f) ? Spec.Duration : Def->DefaultDuration;

	FActiveStatus* Existing = Active.Find(Spec.StatusId);
	if (!Existing)
	{
		FActiveStatus S;
		S.StatusId = Spec.StatusId;
		S.Remaining = UseDuration;
		S.Stacks = FMath::Clamp(Spec.StackDelta, 1, Def->MaxStacks);
		S.bIsCC = Def->bIsCC;
		S.LastSourceTag = Spec.SourceTag;
		Active.Add(Spec.StatusId, S);

		OnStatusApplied.Broadcast(Spec.StatusId, S);
		RecomputeCC();
		return FJRPGOpResult::Ok();
	}

	// already exists → stack policy
	switch (Def->StackPolicy)
	{
	case EStatusStackPolicy::RefreshDuration:
		Existing->Remaining = FMath::Max(Existing->Remaining, UseDuration);
		Existing->LastSourceTag = Spec.SourceTag;
		break;
	case EStatusStackPolicy::AddStack:
		Existing->Stacks = FMath::Clamp(Existing->Stacks + Spec.StackDelta, 1, Def->MaxStacks);
		Existing->Remaining = FMath::Max(Existing->Remaining, UseDuration);
		Existing->LastSourceTag = Spec.SourceTag;
		break;
	case EStatusStackPolicy::IgnoreIfExists:
	default:
		break;
	}

	OnStatusApplied.Broadcast(Spec.StatusId, *Existing);
	RecomputeCC();
	return FJRPGOpResult::Ok();
}

FJRPGOpResult UStatusComponent::RemoveStatus(FName StatusId, FName /*ReasonTag*/)
{
	if (!Active.Remove(StatusId))
		return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Status.NotActive"));

	OnStatusRemoved.Broadcast(StatusId);
	RecomputeCC();
	return FJRPGOpResult::Ok();
}

void UStatusComponent::RecomputeCC()
{
	const bool bPrev = bAnyCC;
	bAnyCC = false;

	for (const auto& It : Active)
	{
		if (It.Value.bIsCC)
		{
			bAnyCC = true;
			break;
		}
	}

	if (bPrev != bAnyCC)
	{
		// Tag 기반(임시)도 함께 제공: CombatMotion이 Tag로 CC 체크 가능
		if (AActor* Owner = GetOwner())
		{
			if (bAnyCC) Owner->Tags.AddUnique("CC");
			else Owner->Tags.Remove("CC");
		}

		OnCCStateChanged.Broadcast(bAnyCC);
	}
}

void UStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// RealTime delta
	const double Now = FPlatformTime::Seconds();
	const float RealDelta = (float)FMath::Max(0.0, Now - LastRealTime);
	LastRealTime = Now;

	if (RealDelta <= 0.f || Active.Num() == 0) return;

	TArray<FName> ToRemove;
	ToRemove.Reserve(Active.Num());

	for (auto& It : Active)
	{
		FActiveStatus& S = It.Value;
		S.Remaining -= RealDelta;
		if (S.Remaining <= 0.f)
		{
			ToRemove.Add(It.Key);
		}
	}

	for (const FName& Id : ToRemove)
	{
		Active.Remove(Id);
		OnStatusRemoved.Broadcast(Id);
	}

	if (ToRemove.Num() > 0)
	{
		RecomputeCC();
	}
}
