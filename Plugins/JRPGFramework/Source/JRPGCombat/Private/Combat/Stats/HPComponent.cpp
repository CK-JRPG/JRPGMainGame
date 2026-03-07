#include "Combat/Stats/HPComponent.h"

#include "Combat/Core/CombatLog.h"
#include "Combat/Infrastructure/BattleSessionSubsystem.h"

#include "Combat/Movement/CombatMotionComponent.h"
#include "Combat/Status/StatusComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"

#include "Combat/SP/SPEventRouterSubsystem.h" // optional router

UHPComponent::UHPComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHPComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOptionalComponents();

	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void UHPComponent::CacheOptionalComponents()
{
	if (!GetOwner()) return;

	CombatMotion = GetOwner()->FindComponentByClass<UCombatMotionComponent>();
	Status = GetOwner()->FindComponentByClass<UStatusComponent>();
	Groggy = GetOwner()->FindComponentByClass<UGroggyComponent>();
	Threat = GetOwner()->FindComponentByClass<UThreatComponent>();
}

void UHPComponent::SetFullHP()
{
	bDead = false;
	CurrentHP = MaxHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void UHPComponent::SetHP(float NewHP)
{
	const float Old = CurrentHP;
	const float MaxAllowed = bCanOverheal ? MaxHP * FMath::Max(1.f, OverhealMaxMultiplier) : MaxHP;

	CurrentHP = FMath::Clamp(NewHP, 0.f, MaxAllowed);
	OnHPChanged.Broadcast(CurrentHP, MaxHP);

	if (Old > 0.f && CurrentHP <= 0.f && !bDead)
	{
		Kill(nullptr, "HP.SetToZero");
	}
}

bool UHPComponent::CheckCombatActiveIfRequired(const FCombatDamageSpec& Spec, FJRPGReason& OutReason) const
{
	if (!Spec.bRequireCombatActive) return true;

	UWorld* W = GetWorld();
	if (!W)
	{
		OutReason = FJRPGReason::Make("Combat.NoWorld");
		return false;
	}

	if (UBattleSessionSubsystem* Battle = W->GetSubsystem<UBattleSessionSubsystem>())
	{
		if (!Battle->IsCombatRunning())
		{
			OutReason = FJRPGReason::Make("Combat.NotRunning");
			return false;
		}
		return true;
	}

	// 전투 세션이 없다면 전투중 체크 불가 → 거부
	OutReason = FJRPGReason::Make("Combat.NoBattleSession");
	return false;
}

FCombatDamageResult UHPComponent::ApplyDamageSpec(const FCombatDamageSpec& Spec)
{
	if (UBattleSessionSubsystem* Session = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		if (Session->ShouldGateEnemyToAlly(Spec.Instigator, GetOwner()))
		{
			FCombatDamageResultR;
			R.Op = FJRPGOpResult::Ok(); // “무시”가 아니라 “게이트로 0 처리” (연출은 선택)
			R.AppliedAmount =0;
			R.OldValue =CurrentHP;
			R.NewValue =CurrentHP;
			returnR;
		}
	}
	
	FCombatDamageResult Result;

	if (bDead)
	{
		Result.Op = FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("HP.Dead"));
		return Result;
	}

	if (Spec.Amount <= 0.f)
	{
		Result.Op = FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("HP.InvalidAmount"));
		return Result;
	}

	FJRPGReason Reason;
	if (!CheckCombatActiveIfRequired(Spec, Reason))
	{
		Result.Op = FJRPGOpResult::Fail(EJRPGResultCode::Rejected, Reason);
		return Result;
	}

	// Damage or Heal
	if (Spec.Kind == ECombatDamageKind::Damage)
	{
		Result = ApplyDamageInternal(Spec);
		if (Result.Op.bOk)
		{
			OnDamaged.Broadcast(Spec, Result);

			// 연동
			ApplyHitReactionIfNeeded(Spec);
			ApplyStatusIfNeeded(Spec);
			ApplyGroggyIfNeeded(Spec);
			ApplyThreatIfNeeded(Spec);
			RouteSPEvents(Spec, Result);

			// CombatMotion CancelPolicy: 피해가 들어왔음을 알려 이동 중단을 자동화
			if (CombatMotion)
			{
				CombatMotion->NotifyExternalHit(Spec.SourceTag.IsNone() ? FName("Hit.Damage") : Spec.SourceTag);
			}
		}
		return Result;
	}

	Result = ApplyHealInternal(Spec);
	if (Result.Op.bOk)
	{
		OnHealed.Broadcast(Spec, Result);
		RouteSPEvents(Spec, Result);
	}
	return Result;
}

FCombatDamageResult UHPComponent::ApplyDamageInternal(const FCombatDamageSpec& Spec)
{
	FCombatDamageResult R;
	R.OldValue = CurrentHP;

	const float Dmg = Spec.Amount;
	const float NewHP = FMath::Clamp(CurrentHP - Dmg, 0.f, MaxHP * FMath::Max(1.f, OverhealMaxMultiplier));

	CurrentHP = NewHP;
	R.AppliedAmount = Dmg;
	R.NewValue = CurrentHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);

	if (CurrentHP <= 0.f && !bDead)
	{
		R.bKilled = true;
		R.EndReasonTag = "HP.Killed";
		Kill(Spec.Instigator, Spec.SourceTag.IsNone() ? FName("Damage") : Spec.SourceTag);
	}
	return R;
}

FCombatDamageResult UHPComponent::ApplyHealInternal(const FCombatDamageSpec& Spec)
{
	FCombatDamageResult R;
	R.OldValue = CurrentHP;

	const float MaxAllowed = bCanOverheal ? MaxHP * FMath::Max(1.f, OverhealMaxMultiplier) : MaxHP;
	const float Heal = Spec.Amount;

	CurrentHP = FMath::Clamp(CurrentHP + Heal, 0.f, MaxAllowed);

	R.AppliedAmount = Heal;
	R.NewValue = CurrentHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);

	// Heal은 kill 없음
	return R;
}

void UHPComponent::Kill(AActor* Killer, FName ReasonTag)
{
	bDead = true;
	CurrentHP = 0.f;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);

	UE_LOG(LogJRPGCombat, Log, TEXT("[HP] %s died. Killer=%s Reason=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Killer),
		*ReasonTag.ToString());

	OnDied.Broadcast(Killer, ReasonTag);
}

void UHPComponent::ApplyHitReactionIfNeeded(const FCombatDamageSpec& Spec)
{
	if (!Spec.bRequestHitMotion) return;
	if (!CombatMotion) return;

	// HitNormal이 없으면 리액션 skip
	const FVector N = Spec.HitWorldNormal.GetSafeNormal();
	if (N.IsNearlyZero()) return;

	// 피해 방향 반대(피해를 준 쪽으로 밀림)
	const FVector Dir = (-N).GetSafeNormal();

	FCombatMotionRequest Req;
	Req.Type = ECombatMotionType::HitMove;
	Req.OwnerTag = "Hit.React";
	Req.DebugTag = Spec.SourceTag;

	switch (Spec.HitReaction)
	{
	case ECombatHitReaction::None:
		return;

	case ECombatHitReaction::Knockback:
		Req.Archetype = ECombatMotionArchetype::Knockback;
		Req.ExecMode = ECombatMotionExecMode::TimeElapsed;
		Req.Direction = Dir;
		Req.Distance = Spec.HitDistance;
		Req.Duration = Spec.HitDuration;
		Req.EndPolicy = ECombatMotionEndPolicy::StopOnBlock;
		Req.bIgnoreFriction = false;
		break;

	case ECombatHitReaction::KnockdownSlide:
		Req.Archetype = ECombatMotionArchetype::KnockdownSlide;
		Req.ExecMode = ECombatMotionExecMode::DistanceReached;
		Req.Direction = Dir;
		Req.Distance = Spec.HitDistance * 1.4f;
		Req.Duration = FMath::Max(0.2f, Spec.HitDuration * 2.0f);
		Req.EndPolicy = ECombatMotionEndPolicy::StopOnBlock;
		Req.bIgnoreFriction = true;
		break;

	case ECombatHitReaction::WallSlam:
		Req.Archetype = ECombatMotionArchetype::WallSlam;
		Req.ExecMode = ECombatMotionExecMode::DistanceReached;
		Req.Direction = Dir;
		Req.Distance = Spec.WallSlamDistance;
		Req.Duration = Spec.WallSlamDuration;
		Req.EndPolicy = ECombatMotionEndPolicy::StopOnBlock;
		Req.bIgnoreFriction = true;
		break;

	case ECombatHitReaction::Knockup:
		Req.Archetype = ECombatMotionArchetype::Knockup;
		Req.ExecMode = ECombatMotionExecMode::Launch;
		Req.LaunchVelocity = FVector(Dir.X * 200.f, Dir.Y * 200.f, Spec.LaunchUpZ);
		Req.LaunchMaxTime = Spec.LaunchMaxTime;
		Req.bEndLaunchWhenGrounded = true;
		break;

	case ECombatHitReaction::SlamToGround:
		Req.Archetype = ECombatMotionArchetype::SlamToGround;
		Req.ExecMode = ECombatMotionExecMode::Launch;
		Req.LaunchVelocity = FVector(0.f, 0.f, Spec.SlamDownZ);
		Req.LaunchMaxTime = FMath::Max(0.2f, Spec.LaunchMaxTime);
		Req.bEndLaunchWhenGrounded = true;
		break;

	default:
		break;
	}

	CombatMotion->RequestCombatMotion(Req);
}

void UHPComponent::ApplyStatusIfNeeded(const FCombatDamageSpec& Spec)
{
	if (!Spec.bApplyStatus) return;
	if (!Status) return;
	if (Spec.StatusId.IsNone()) return;

	FStatusSpec S;
	S.StatusId = Spec.StatusId;
	S.Duration = Spec.StatusDuration;
	S.SourceTag = Spec.SourceTag.IsNone() ? FName("Damage.Status") : Spec.SourceTag;
	S.Instigator = Spec.Instigator;

	Status->ApplyStatus(S);
}

void UHPComponent::ApplyGroggyIfNeeded(const FCombatDamageSpec& Spec)
{
	if (!Groggy) return;
	if (Spec.BreakAmount <= 0.f) return;

	Groggy->AddBreak(Spec.BreakAmount, Spec.SourceTag.IsNone() ? FName("Damage.Break") : Spec.SourceTag);
}

void UHPComponent::ApplyThreatIfNeeded(const FCombatDamageSpec& Spec)
{
	if (!Threat) return;
	if (!Spec.Instigator) return;
	if (Spec.ThreatAmount <= 0.f) return;

	Threat->AddThreat(Spec.Instigator, Spec.ThreatAmount, Spec.SourceTag.IsNone() ? FName("Threat.Damage") : Spec.SourceTag);
}

void UHPComponent::RouteSPEvents(const FCombatDamageSpec& Spec, const FCombatDamageResult& Result)
{
	// (1) 라우터 서브시스템이 있으면 그쪽으로
	if (UWorld* W = GetWorld())
	{
		if (USPEventRouterSubsystem* Router = W->GetSubsystem<USPEventRouterSubsystem>())
		{
			Router->RouteDamageEvent(Spec, Result, GetOwner());
			return;
		}
	}
	// (2) 라우터가 없으면 아무 것도 안 함
}
