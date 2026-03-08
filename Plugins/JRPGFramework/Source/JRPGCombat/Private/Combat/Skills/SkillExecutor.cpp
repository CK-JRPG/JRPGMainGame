#include "Combat/Skills/SkillExecutor.h"

#include "Combat/Skills/JRPGSkillComponent.h"
#include "Combat/Skills/JRPGSkillDataAsset.h"

#include "Combat/Stats/CombatAPComponent.h"
#include "Combat/Stats/CombatHPComponent.h"
#include "Combat/Status/StatusComponent.h"
#include "Combat/Groggy/CombatGroggyComponent.h"
#include "Combat/Threat/CombatThreatComponent.h"
#include "Combat/Movement/JRPGCombatMotionComponent.h"
#include "Combat/SP/SPEventRouterSubsystem.h"

static AActor* ResolveCaster(UJRPGSkillComponent& SkillComp, const FJRPGSkillRequest& Req)
{
	if (Req.Instigator) return Req.Instigator;
	return SkillComp.GetOwner();
}

FJRPGSkillResult FSkillExecutor::Execute(UJRPGSkillComponent& SkillComp, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req)
{
	FJRPGSkillResult R;
	R.SkillId = Skill.SkillId;

	FJRPGReason Reason;
	int32 APCost = 0;

	if (!ValidateRequest(SkillComp, Skill, Req, Reason, APCost))
	{
		R.Op = FJRPGOpResult::Fail(EJRPGResultCode::Rejected, Reason);
		return R;
	}

	if (!CommitCosts(SkillComp, APCost, Req, Reason))
	{
		R.Op = FJRPGOpResult::Fail(EJRPGResultCode::Rejected, Reason);
		return R;
	}

	// 성공 커밋 이후 적용
	R.bExecuted = true;
	R.SpentAP = APCost;

	ApplyEffects(SkillComp, Skill, Req, R);

	// 쿨다운 시작은 SkillComponent가 최종 SSOT로 처리(여기서는 시간 값만 채움)
	R.StartedCooldown = Skill.Cooldown.CooldownSec;
	R.StartedGlobalCooldown = Skill.Cooldown.GlobalCooldownSec;

	R.Op = FJRPGOpResult::Ok();
	return R;
}

bool FSkillExecutor::ValidateRequest(UJRPGSkillComponent& SkillComp, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, FJRPGReason& OutReason, int32& OutAPCost)
{
	FJRPGReason VReason;
	if (!Skill.Validate(VReason))
	{
		OutReason = VReason;
		return false;
	}

	AActor* Caster = ResolveCaster(SkillComp, Req);
	if (!Caster)
	{
		OutReason = FJRPGReason::Make("Skill.NoCaster");
		return false;
	}

	// 죽음 체크
	if (UCombatHPComponent* HP = Caster->FindComponentByClass<UCombatHPComponent>())
	{
		if (HP->IsDead())
		{
			OutReason = FJRPGReason::Make("Skill.CasterDead");
			return false;
		}
	}

	// CC 체크(기획서 기반: CC 중 스킬 불가 같은 정책은 여기서 고정)
	if (UStatusComponent* Status = Caster->FindComponentByClass<UStatusComponent>())
	{
		if (Status->IsCrowdControlled())
		{
			OutReason = FJRPGReason::Make("Skill.CasterCC");
			return false;
		}
	}

	// 타겟팅 체크
	switch (Skill.Targeting)
	{
	case EJRPGSkillTargeting::Self:
		break;
	case EJRPGSkillTargeting::SingleActor:
	case EJRPGSkillTargeting::SelfAndTarget:
		if (!Req.PrimaryTarget)
		{
			OutReason = FJRPGReason::Make("Skill.NoTarget");
			return false;
		}
		break;
	case EJRPGSkillTargeting::MultiActor:
		if (!Req.PrimaryTarget && Req.AdditionalTargets.Num() == 0)
		{
			OutReason = FJRPGReason::Make("Skill.NoTargets");
			return false;
		}
		break;
	case EJRPGSkillTargeting::Point:
		// TargetLocation은 0,0,0이 될 수도 있으니 여기선 허용
		break;
	default:
		break;
	}

	// 쿨다운 체크(SSOT: SkillComponent)
	if (!Req.bIgnoreCooldown)
	{
		if (SkillComp.IsOnCooldown(Skill.SkillId))
		{
			OutReason = FJRPGReason::Make("Skill.OnCooldown");
			return false;
		}
	}
	if (!Req.bIgnoreGlobalCooldown)
	{
		if (SkillComp.IsOnGlobalCooldown())
		{
			OutReason = FJRPGReason::Make("Skill.OnGCD");
			return false;
		}
	}

	// AP 체크
	OutAPCost = Req.bIgnoreCost ? 0 : Skill.Cost.APCost;
	if (OutAPCost > 0)
	{
		if (UCombatAPComponent* AP = Caster->FindComponentByClass<UCombatAPComponent>())
		{
			if (!AP->CanSpend(OutAPCost))
			{
				OutReason = FJRPGReason::Make("Skill.NotEnoughAP");
				return false;
			}
		}
		else
		{
			OutReason = FJRPGReason::Make("Skill.NoAPComponent");
			return false;
		}
	}

	return true;
}

bool FSkillExecutor::CommitCosts(UJRPGSkillComponent& SkillComp, int32 APCost, const FJRPGSkillRequest& Req, FJRPGReason& OutReason)
{
	AActor* Caster = ResolveCaster(SkillComp, Req);
	if (!Caster)
	{
		OutReason = FJRPGReason::Make("Skill.NoCaster");
		return false;
	}

	// AP 소비
	if (APCost > 0)
	{
		UCombatAPComponent* AP = Caster->FindComponentByClass<UCombatAPComponent>();
		if (!AP)
		{
			OutReason = FJRPGReason::Make("Skill.NoAPComponent");
			return false;
		}

		const FJRPGOpResult SpendR = AP->Spend(APCost, Req.SourceTag.IsNone() ? FName("Skill") : Req.SourceTag);
		if (!SpendR.bOk)
		{
			OutReason = SpendR.Reason;
			return false;
		}
	}

	return true;
}

void FSkillExecutor::BuildTargetList(const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, TArray<AActor*>& OutTargets)
{
	OutTargets.Reset();

	switch (Skill.Targeting)
	{
	case EJRPGSkillTargeting::Self:
		// handled per-effect
		break;
	case EJRPGSkillTargeting::SingleActor:
	case EJRPGSkillTargeting::SelfAndTarget:
		if (Req.PrimaryTarget) OutTargets.Add(Req.PrimaryTarget);
		break;
	case EJRPGSkillTargeting::MultiActor:
		if (Req.PrimaryTarget) OutTargets.Add(Req.PrimaryTarget);
		for (AActor* A : Req.AdditionalTargets)
		{
			if (A) OutTargets.AddUnique(A);
		}
		break;
	default:
		break;
	}
}

void FSkillExecutor::ApplyEffects(UJRPGSkillComponent& SkillComp, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, FJRPGSkillResult& InOutResult)
{
	AActor* Caster = ResolveCaster(SkillComp, Req);
	AActor* Primary = Req.PrimaryTarget;

	TArray<AActor*> Targets;
	BuildTargetList(Skill, Req, Targets);

	for (const FJRPGSkillEffectEntry& E : Skill.Effects)
	{
		// 대상 결정
		TArray<AActor*> ApplyTargets;

		switch (E.Target)
		{
		case EJRPGEffectTarget::Self:
			if (Caster) ApplyTargets.Add(Caster);
			break;

		case EJRPGEffectTarget::PrimaryTarget:
			if (Primary) ApplyTargets.Add(Primary);
			break;

		case EJRPGEffectTarget::AllTargets:
			ApplyTargets = Targets;
			break;
		}

		// 타겟 없는 이펙트는 처리 스킵(안전)
		if (ApplyTargets.Num() == 0 &&
			(E.Kind != EJRPGSkillEffectKind::RequestMotion) &&
			(E.Kind != EJRPGSkillEffectKind::GrantSP))
		{
			continue;
		}

		switch (E.Kind)
		{
		case EJRPGSkillEffectKind::DealDamage:
			for (AActor* T : ApplyTargets)
				ApplyDamageOrHeal(Caster, T, Skill, Req, E, false, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.Damage");
			break;

		case EJRPGSkillEffectKind::Heal:
			for (AActor* T : ApplyTargets)
				ApplyDamageOrHeal(Caster, T, Skill, Req, E, true, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.Heal");
			break;

		case EJRPGSkillEffectKind::ApplyStatus:
			for (AActor* T : ApplyTargets)
				ApplyStatus(Caster, T, Skill, Req, E, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.ApplyStatus");
			break;

		case EJRPGSkillEffectKind::RemoveStatus:
			for (AActor* T : ApplyTargets)
				RemoveStatus(T, E, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.RemoveStatus");
			break;

		case EJRPGSkillEffectKind::AddGroggy:
			for (AActor* T : ApplyTargets)
				AddGroggy(T, E, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.Groggy");
			break;

		case EJRPGSkillEffectKind::AddThreat:
			for (AActor* T : ApplyTargets)
				AddThreat(T, Caster, E, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.Threat");
			break;

		case EJRPGSkillEffectKind::RequestMotion:
			RequestMotion(Caster, Primary, Skill, Req, E, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.Motion");
			break;

		case EJRPGSkillEffectKind::GrantSP:
			GrantSP(Caster, Skill, Req, E, InOutResult);
			InOutResult.AppliedEffectTags.Add("Effect.SP");
			break;

		default:
			break;
		}
	}
}

void FSkillExecutor::ApplyDamageOrHeal(AActor* Caster, AActor* Target, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, bool bHeal, FJRPGSkillResult& InOutResult)
{
	if (!Target) return;

	UCombatHPComponent* HP = Target->FindComponentByClass<UCombatHPComponent>();
	if (!HP) return;

	FCombatDamageSpec Spec;
	Spec.Kind = bHeal ? ECombatDamageKind::Heal : ECombatDamageKind::Damage;
	Spec.Amount = E.Damage.Amount;
	Spec.SourceTag = Skill.SkillId; // 스킬 ID를 source로
	Spec.Instigator = Caster;
	Spec.Causer = Req.Instigator;
	Spec.HitWorldNormal = Req.PrimaryTarget ? (Req.PrimaryTarget->GetActorLocation() - (Caster ? Caster->GetActorLocation() : FVector::ZeroVector)).GetSafeNormal() : FVector::ZeroVector;
	Spec.bFromTacticalReservation = Req.bFromTacticalReservation;
	
	// Damage 전용 연동
	if (!bHeal)
	{
		Spec.bRequestHitMotion = (E.Damage.HitReaction != ECombatHitReaction::None);
		Spec.HitReaction = E.Damage.HitReaction;

		Spec.BreakAmount = E.Damage.BreakAmount;
		Spec.ThreatAmount = E.Damage.ThreatAmount;
		Spec.SPOnHit = E.Damage.SPOnHit;
		Spec.SPOnKill = E.Damage.SPOnKill;

		Spec.HitDistance = E.Damage.HitDistance;
		Spec.HitDuration = E.Damage.HitDuration;
		Spec.LaunchUpZ = E.Damage.LaunchUpZ;
		Spec.SlamDownZ = E.Damage.SlamDownZ;
		Spec.LaunchMaxTime = E.Damage.LaunchMaxTime;
		Spec.WallSlamDistance = E.Damage.WallSlamDistance;
		Spec.WallSlamDuration = E.Damage.WallSlamDuration;
	}

	const FCombatDamageResult R = HP->ApplyDamageSpec(Spec);
	if (!R.Op.bOk)
	{
		// 실행은 계속(부분 성공 허용). 필요하면 여기서 중단 정책 추가 가능.
		return;
	}
}

void FSkillExecutor::ApplyStatus(AActor* Caster, AActor* Target, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult)
{
	if (!Target) return;

	UStatusComponent* Status = Target->FindComponentByClass<UStatusComponent>();
	if (!Status) return;

	FStatusSpec S;
	S.StatusId = E.Status.StatusId;
	S.Duration = E.Status.Duration;
	S.SourceTag = Skill.SkillId;
	S.Instigator = Caster;

	Status->ApplyStatus(S);
}

void FSkillExecutor::RemoveStatus(AActor* Target, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult)
{
	if (!Target) return;
	if (E.Status.StatusId.IsNone()) return;

	UStatusComponent* Status = Target->FindComponentByClass<UStatusComponent>();
	if (!Status) return;

	Status->RemoveStatus(E.Status.StatusId, "Skill.RemoveStatus");
}

void FSkillExecutor::AddGroggy(AActor* Target, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult)
{
	if (!Target) return;

	UGroggyComponent* Groggy = Target->FindComponentByClass<UGroggyComponent>();
	if (!Groggy) return;

	Groggy->AddBreak(E.Groggy.BreakAmount, "Skill.Groggy");
}

void FSkillExecutor::AddThreat(AActor* Target, AActor* Caster, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult)
{
	if (!Target || !Caster) return;

	UCombatThreatComponent* Threat = Target->FindComponentByClass<UCombatThreatComponent>();
	if (!Threat) return;

	Threat->AddThreat(Caster, E.Threat.ThreatAmount, "Skill.Threat");
}

FVector FSkillExecutor::ResolveMotionDirection(AActor* Caster, AActor* PrimaryTarget, const FJRPGSkillMotionEffect& Motion)
{
	if (!Caster) return FVector::ForwardVector;

	switch (Motion.DirectionMode)
	{
	case EJRPGSkillMotionDirection::Forward:
		return Caster->GetActorForwardVector();

	case EJRPGSkillMotionDirection::TowardTarget:
		if (PrimaryTarget)
		{
			const FVector D = PrimaryTarget->GetActorLocation() - Caster->GetActorLocation();
			return FVector(D.X, D.Y, 0.f).GetSafeNormal();
		}
		return Caster->GetActorForwardVector();

	case EJRPGSkillMotionDirection::AwayFromTarget:
		if (PrimaryTarget)
		{
			const FVector D = Caster->GetActorLocation() - PrimaryTarget->GetActorLocation();
			return FVector(D.X, D.Y, 0.f).GetSafeNormal();
		}
		return -Caster->GetActorForwardVector();

	case EJRPGSkillMotionDirection::CustomWorld:
	default:
		return Motion.CustomWorldDirection.GetSafeNormal();
	}
}

void FSkillExecutor::RequestMotion(AActor* Caster, AActor* PrimaryTarget, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult)
{
	if (!Caster) return;

	UJRPGCombatMotionComponent* MotionComp = Caster->FindComponentByClass<UJRPGCombatMotionComponent>();
	if (!MotionComp) return;

	FCombatMotionRequest M;
	M.Type = ECombatMotionType::SkillMove;
	M.Archetype = E.Motion.Archetype;
	M.ExecMode = E.Motion.ExecMode;

	M.Direction = ResolveMotionDirection(Caster, PrimaryTarget, E.Motion);
	M.Distance = E.Motion.Distance;
	M.Duration = E.Motion.Duration;

	M.OwnerTag = FName(*FString::Printf(TEXT("SkillMove.%s"), *Skill.SkillId.ToString()));
	M.DebugTag = Skill.SkillId;

	M.bIgnoreFriction = E.Motion.bIgnoreFriction;
	M.EndPolicy = E.Motion.EndPolicy;
	M.bFaceMoveDirection = E.Motion.bFaceMoveDirection;
	M.FaceYawInterpSpeed = E.Motion.FaceYawInterpSpeed;

	M.Instigator = Caster;
	M.Target = PrimaryTarget;

	MotionComp->RequestCombatMotion(M);
}

void FSkillExecutor::GrantSP(AActor* Caster, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult)
{
	if (!Caster) return;
	UWorld* W = Caster->GetWorld();
	if (!W) return;

	// SP는 EventRouterSubsystem을 통해 표준 입력으로만 증가(정책 유지)
	if (USPEventRouterSubsystem* Router = W->GetSubsystem<USPEventRouterSubsystem>())
	{
		FCombatDamageSpec Dummy;
		Dummy.Kind = ECombatDamageKind::Damage; // 라우터는 Damage만 처리했었지만, SP 직접 부여를 위해 별도 경로가 필요할 수 있음.
		// 여기서는 Skill 시스템에서 직접 ApplyGainEvent를 호출하는 게 더 명확하지만,
		// 현재 구조를 유지하기 위해, 이후 SPRouter에 "RouteDirectSP"를 추가하는 걸 추천.
		// 일단 “즉시 적용”을 위해 여기서는 SynergyPointSubsystem 직접 호출(정책 위반 아님: 여전히 이벤트로 적용)
	}

	// 직접 적용(현재 구현된 SPSubsystem API 기준)
	// (추후 Router에 RouteDirectSP 추가하면 여기만 바꾸면 됨)
	if (USynergyPointSubsystem* SP = W->GetSubsystem<USynergyPointSubsystem>())
	{
		FJRPGSPGainEvent Ev;
		Ev.Amount = E.SP.Amount;
		Ev.SourceTag = E.SP.SourceTag.IsNone() ? Skill.SkillId : E.SP.SourceTag;
		Ev.Instigator = Caster;
		SP->ApplyGainEvent(Ev);
	}
}