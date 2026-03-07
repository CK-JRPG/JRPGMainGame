#include "Combat/Integration/CombatMotionEffectBridgeComponent.h"

#include "Combat/Movement/CombatMotionComponent.h"
#include "Combat/Status/StatusComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Infrastructure/SynergyPointSubsystem.h"
#include "Combat/Integration/CombatMotionImpactConfigDataAsset.h"

UCombatMotionEffectBridgeComponent::UCombatMotionEffectBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatMotionEffectBridgeComponent::BeginPlay()
{
	Super::BeginPlay();

	Motion = GetOwner() ? GetOwner()->FindComponentByClass<UCombatMotionComponent>() : nullptr;
	Status = GetOwner() ? GetOwner()->FindComponentByClass<UStatusComponent>() : nullptr;
	Groggy = GetOwner() ? GetOwner()->FindComponentByClass<UGroggyComponent>() : nullptr;
	Threat = GetOwner() ? GetOwner()->FindComponentByClass<UThreatComponent>() : nullptr;

	Bind();
}

void UCombatMotionEffectBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unbind();
	RequestById.Reset();
	Super::EndPlay(EndPlayReason);
}

void UCombatMotionEffectBridgeComponent::Bind()
{
	if (!Motion) return;

	Motion->OnMotionStarted.AddUObject(this, &UCombatMotionEffectBridgeComponent::HandleMotionStarted);
	Motion->OnMotionEnded.AddUObject(this, &UCombatMotionEffectBridgeComponent::HandleMotionEnded);
	Motion->OnMotionCancelled.AddUObject(this, &UCombatMotionEffectBridgeComponent::HandleMotionCancelled);

	// 벽꿍 이벤트(CombatMotion이 발행)
	Motion->OnWallSlam.AddUObject(this, &UCombatMotionEffectBridgeComponent::HandleWallSlam);

	// Status CC 변화 -> CombatMotion CancelPolicy(OnCC) 자동 트리거
	if (Status)
	{
		Status->OnCCStateChanged.AddUObject(this, &UCombatMotionEffectBridgeComponent::HandleCCChanged);
	}
}

void UCombatMotionEffectBridgeComponent::Unbind()
{
	if (!Motion) return;

	Motion->OnMotionStarted.RemoveAll(this);
	Motion->OnMotionEnded.RemoveAll(this);
	Motion->OnMotionCancelled.RemoveAll(this);
	Motion->OnWallSlam.RemoveAll(this);

	if (Status)
	{
		Status->OnCCStateChanged.RemoveAll(this);
	}
}

void UCombatMotionEffectBridgeComponent::HandleMotionStarted(const FCombatMotionHandle& H, const FCombatMotionRequest& Req)
{
	RequestById.Add(H.UniqueId, Req);
}

void UCombatMotionEffectBridgeComponent::HandleMotionEnded(const FCombatMotionHandle& H, FName EndReason)
{
	const FCombatMotionRequest* Req = RequestById.Find(H.UniqueId);
	if (Req)
	{
		// Slam 착지 같은 “EndReason 기반” 처리
		if (Req->Archetype == ECombatMotionArchetype::SlamToGround &&
			(EndReason == "End.SlamGrounded" || EndReason == "End.Grounded"))
		{
			ApplyImpactRule(ECombatMotionArchetype::SlamToGround, *Req, "CombatMotion.Slam");
		}
		// KnockdownSlide 종료 시 추가 처리하고 싶으면 여기서 가능
	}
	RequestById.Remove(H.UniqueId);
}

void UCombatMotionEffectBridgeComponent::HandleMotionCancelled(const FCombatMotionHandle& H, FName /*CancelReason*/)
{
	RequestById.Remove(H.UniqueId);
}

void UCombatMotionEffectBridgeComponent::HandleWallSlam(const FCombatMotionHandle& H, const FHitResult& /*Hit*/, FName /*WallSlamTag*/)
{
	const FCombatMotionRequest* Req = RequestById.Find(H.UniqueId);
	if (!Req) return;

	// 벽꿍 효과 적용
	ApplyImpactRule(ECombatMotionArchetype::WallSlam, *Req, "CombatMotion.WallSlam");
}

void UCombatMotionEffectBridgeComponent::HandleCCChanged(bool bNowCC)
{
	// CombatMotion은 OnCC CancelPolicy가 있을 때만 취소되게 되어 있음
	if (Motion)
	{
		Motion->NotifyExternalCCStateChanged(bNowCC);
	}
}

FCombatMotionImpactRule UCombatMotionEffectBridgeComponent::DefaultRuleFor(ECombatMotionArchetype Archetype) const
{
	FCombatMotionImpactRule R;

	switch (Archetype)
	{
	case ECombatMotionArchetype::WallSlam:
		R.bApplyStatus = true;  R.StatusId = "Status.Stun"; R.StatusDuration = 1.2f;
		R.bAddGroggy = true;   R.GroggyBreakAmount = 35.f;
		R.bGrantSP = true;     R.SPAmount = 12;
		R.bLockThreatToInstigator = true; R.ThreatLockDuration = 1.0f;
		return R;

	case ECombatMotionArchetype::SlamToGround:
		R.bApplyStatus = true;  R.StatusId = "Status.Stun"; R.StatusDuration = 0.8f;
		R.bAddGroggy = true;   R.GroggyBreakAmount = 30.f;
		R.bGrantSP = true;     R.SPAmount = 10;
		return R;

	case ECombatMotionArchetype::KnockdownSlide:
		R.bApplyStatus = true;  R.StatusId = "Status.Knockdown"; R.StatusDuration = 1.0f;
		R.bAddGroggy = true;   R.GroggyBreakAmount = 20.f;
		return R;

	case ECombatMotionArchetype::Knockup:
		R.bApplyStatus = true;  R.StatusId = "Status.Airborne"; R.StatusDuration = 0.8f;
		return R;

	default:
		return R;
	}
}

void UCombatMotionEffectBridgeComponent::ApplyImpactRule(ECombatMotionArchetype Archetype, const FCombatMotionRequest& Req, FName SourceTag)
{
	FCombatMotionImpactRule Rule;
	const bool bHasConfig = (ImpactConfig && ImpactConfig->TryGetRule(Archetype, Rule));
	if (!bHasConfig)
	{
		Rule = DefaultRuleFor(Archetype);
	}

	// 1) Status
	if (Rule.bApplyStatus && Status && !Rule.StatusId.IsNone())
	{
		FStatusSpec S;
		S.StatusId = Rule.StatusId;
		S.Duration = Rule.StatusDuration;
		S.SourceTag = SourceTag;
		S.Instigator = Req.Instigator;
		Status->ApplyStatus(S);
	}

	// 2) Groggy
	if (Rule.bAddGroggy && Groggy)
	{
		Groggy->AddBreak(Rule.GroggyBreakAmount, SourceTag);
	}

	// 3) SP (월드 SSOT)
	if (Rule.bGrantSP)
	{
		if (UWorld* W = GetWorld())
		{
			if (USynergyPointSubsystem* SP = W->GetSubsystem<USynergyPointSubsystem>())
			{
				FJRPGSPGainEvent Ev;
				Ev.Amount = Rule.SPAmount;
				Ev.SourceTag = SourceTag;
				Ev.Instigator = Req.Instigator;
				SP->ApplyGainEvent(Ev);
			}
		}
	}

	// 4) Threat (옵션: 피해 유발자에게 잠깐 고정/증가)
	if (Threat)
	{
		if (Rule.bAddThreatToInstigator && Req.Instigator)
		{
			Threat->AddThreat(Req.Instigator, Rule.ThreatAmount, SourceTag);
		}
		if (Rule.bLockThreatToInstigator && Req.Instigator)
		{
			Threat->LockTarget(Req.Instigator, Rule.ThreatLockDuration, SourceTag);
		}
	}

	// 5) CombatMotion CancelPolicy 트리거(피격/CC는 외부에서 별도로 NotifyExternalHit/CC로 처리)
	//    여기선 "효과 적용"만 담당.
}