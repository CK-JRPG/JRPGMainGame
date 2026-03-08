#include "Combat/Infrastructure/TrinityChainSubsystem.h"

#include "Combat/Chain/ChainSettingsDataAsset.h"
#include "Combat/Infrastructure/CombatTimeSubsystem.h"
#include "Combat/Infrastructure/CombatBattleSessionSubsystem.h"
#include "Combat/Infrastructure/CombatSynergyPointSubsystem.h"
#include "Combat/Infrastructure/CombatTacticalModeSubsystem.h"

#include "Combat/Skills/JRPGSkillComponent.h"
#include "Combat/Skills/JRPGSkillDataAsset.h"
#include "Combat/Stats/CombatHPComponent.h"

static double NowReal(const UWorld *W)
{
	return W ?W->GetRealTimeSeconds() :0.0;
}

UCombatTimeSubsystem* UTrinityChainSubsystem::GetTime() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatTimeSubsystem>() : nullptr;
}

UCombatBattleSessionSubsystem* UTrinityChainSubsystem::GetSession() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatBattleSessionSubsystem>() : nullptr;
}

UCombatTacticalModeSubsystem* UTrinityChainSubsystem::GetTactical() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatTacticalModeSubsystem>() : nullptr;
}

UCombatSynergyPointSubsystem* UTrinityChainSubsystem::GetSP() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatSynergyPointSubsystem>() : nullptr;
}

void UTrinityChainSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);

	Settings =SettingsAsset ? SettingsAsset->Settings :FChainSettings();

	Ctx = FChainContext();
	Ctx.State = EChainState::Idle;
	Ctx.Steps.SetNum(3);
}

void UTrinityChainSubsystem::Deinitialize()
{
	// 안전 종료
	if (IsChainActive())
	{
		AbortChain("Chain.Deinit");
	}
	Super::Deinitialize();
}

void UTrinityChainSubsystem::TransitionTo(EChainState NewState)
{
	if (Ctx.State==NewState)return;
	const EChainState Prev = Ctx.State;
	Ctx.State = NewState;
	OnChainStateChanged.Broadcast(Prev,NewState);

	const bool bActive = (Ctx.State == EChainState::SelectionFrozen);
	OnChainTimerUpdated.Broadcast(GetSelectionElapsedRealSec(),GetSelectionRemainingRealSec(),GetSelectionNormalized01(),bActive);
}

bool UTrinityChainSubsystem::GuardCanStart(AActor *PrimaryTarget, FJRPGReason &OutReason)const
{
	UCombatBattleSessionSubsystem *Session = GetSession();
	if (!Session || !Session->IsCombatRunning())
	{
		OutReason = FJRPGReason::Make("SessionNotActive");
		return false;
	}

	if (IsChainActive())
	{
		OutReason = FJRPGReason::Make("Chain.AlreadyActive");
		return false;
	}

	UCombatSynergyPointSubsystem *SP = GetSP();
	if (!SP || !SP->GetState().bChainReady)
	{
		OutReason = FJRPGReason::Make("Chain.NotReadySP");
		return false;
	}

	if (!PrimaryTarget)
	{
		OutReason = FJRPGReason::Make("Chain.NoPrimaryTarget");
		return false;
	}

	// 타겟 살아있음(최소)
	if (UCombatHPComponent *HP = PrimaryTarget->FindComponentByClass<UCombatHPComponent>())
	{
		if (HP->IsDead())
		{
			OutReason = FJRPGReason::Make("Chain.TargetDead");
			return false;
		}
	}

	return true;
}

FJRPGOpResult UTrinityChainSubsystem::TryStartChain()
{
	if (UCombatBattleSessionSubsystem *Session = GetSession())
	{
		return TryStartChainWithTarget(Session->GetPrimaryTarget());
	}
	return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Chain.NoSession"));
}

FJRPGOpResult UTrinityChainSubsystem::TryStartChainWithTarget(AActor *PrimaryTarget)
{
	FJRPGReason Reason;
	if (!GuardCanStart(PrimaryTarget,Reason))
	{
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected,Reason);
	}

	// 전술 모드 강제 종료 옵션
	if (Settings.bExitTacticalOnStart)
	{
		if (UCombatTacticalModeSubsystem *Tactical = GetTactical())
		{
			Tactical->TryExitTactical("Chain.Start");
		}
	}

	BeginSelection(PrimaryTarget);
	return FJRPGOpResult::Ok();
}

void UTrinityChainSubsystem::ApplyEntryGuards()
{
	UCombatBattleSessionSubsystem *Session = GetSession();
	if (!Session)return;

	// 입력 잠금(플레이어 입력만) :contentReference[oaicite:4]{index=4}
	Session->PushPlayerInputLock("Chain");

	// Enemy suppression (세션 소유)
	if (Settings.bSuppressEnemyAttacksDuringChain)
	{
		Session->PushEnemySuppression("Chain", Settings.EnemySuppressionScope);
		Ctx.bEnemySuppressed = true;
	}
}

void UTrinityChainSubsystem::ReleaseEntryGuards()
{
	UCombatBattleSessionSubsystem*Session =GetSession();
	if (!Session)return;

	Session->PopPlayerInputLock("Chain");

	if (Ctx.bEnemySuppressed)
	{
		Session->PopEnemySuppression("Chain");
		Ctx.bEnemySuppressed =false;
	}
}

void UTrinityChainSubsystem::BeginSelection(AActor*PrimaryTarget)
{
	UWorld*W =GetWorld();
	if (!W)return;

	Ctx =FChainContext();
	Ctx.Token = FChainToken::NewToken();
	Ctx.PrimaryTarget =PrimaryTarget;
	Ctx.State = EChainState::Idle;
	Ctx.Steps.SetNum(3);

	Ctx.StartTimeReal =NowReal(W);
	Ctx.SelectionDeadlineReal =Ctx.StartTimeReal+ (double)FMath::Max(0.1f,Settings.SelectionMaxRealSec);

	EnsurePartyActors();

	// Time Freeze 요청(Chain = Critical 우선순위)
	if (UCombatTimeSubsystem*Time =GetTime())
	{
		FCombatTimeRequest Req;
		Req.Mode = ECombatTimeMode::Frozen;
		Req.Priority = ECombatTimePriority::Critical;
		Req.OwnerTag = "Trinity";
		Req.TimeScale = Settings.FrozenTimeScale;
		Req.DurationRealSec = Settings.ChainMaxRealSec;// 만료 안전장치
		Req.BlendInSec = Settings.BlendInSec;
		Req.BlendOutSec = Settings.BlendOutSec;

		FCombatTimeResult TR =Time->RequestTimeMode(Req);
		if (TR.Op.bOk && TR.Handle.IsValid())
		{
			Ctx.TimeHandleValue = TR.Handle.Value;
		}
	}

	ApplyEntryGuards();

	TransitionTo(EChainState::SelectionFrozen);

	OnChainTimerUpdated.Broadcast(0.f,Settings.SelectionMaxRealSec,0.f,true);
}

void UTrinityChainSubsystem::EnsurePartyActors()
{
	Ctx.PartyActors.Reset();

	if (UCombatBattleSessionSubsystem *Session = GetSession())
	{
		TArray<AActor*> Party;
		Session->GetPartyActors(Party);

		// 최대 3인(문서 기반)
		for (int32 i =0; i < Party.Num() && Ctx.PartyActors.Num()<3; ++i)
		{
			Ctx.PartyActors.Add(Party[i]);
		}
	}
}

float UTrinityChainSubsystem::GetSelectionElapsedRealSec() const
{
	if (!GetWorld()) return 0.f;
	
	if (Ctx.State != EChainState::SelectionFrozen)return 0.f;
	return (float) FMath::Max(0.0,NowReal(GetWorld()) -Ctx.StartTimeReal);
}

float UTrinityChainSubsystem::GetSelectionRemainingRealSec() const
{
	if (Ctx.State!= EChainState::SelectionFrozen) return 0.f;
	
	const double Now = NowReal(GetWorld());
	return (float)FMath::Max(0.0f,Ctx.SelectionDeadlineReal - Now);
}

float UTrinityChainSubsystem::GetSelectionNormalized01() const
{
	if (Ctx.State != EChainState::SelectionFrozen) return 0.f;
	
	const float MaxD = FMath::Max(0.001f,Settings.SelectionMaxRealSec);
	return FMath::Clamp(GetSelectionElapsedRealSec() / MaxD,0.f,1.f);
}

bool UTrinityChainSubsystem::IsCasterPickable(AActor *Caster, FName &OutReasonTag) const
{
	if (!Caster)
	{
		OutReasonTag ="Pick.CasterNull";
		return false;
	}

	if (UCombatHPComponent *HP = Caster->FindComponentByClass<UCombatHPComponent>())
	{
		if (HP->IsDead())
		{
			OutReasonTag ="Pick.CasterDead";
			return false;
		}
	}

	// 최소 규칙: 상태이상 태그로 사용 불가(프로젝트 Status 시스템 붙으면 여기서 확장)
	if (Caster->ActorHasTag("CC.Stun")||Caster->ActorHasTag("CC.Silence")||Caster->ActorHasTag("State.CannotAct"))
	{
		OutReasonTag ="Pick.CasterCC";
		return false;
	}

	return true;
}

const UJRPGSkillDataAsset* UTrinityChainSubsystem::FindSkillAsset(AActor *Caster, FName SkillId)const
{
	if (!Caster) return nullptr;
	const UJRPGSkillComponent *SC = Caster->FindComponentByClass<UJRPGSkillComponent>();
	if (!SC) return nullptr;
	return SC->GetSkillAsset(SkillId);// (아래 “필수 패치”에서 추가)
}

bool UTrinityChainSubsystem::IsSkillEligibleForChain(AActor*Caster, FName SkillId, FName&OutReasonTag)const
{
	if (SkillId.IsNone())
	{
		OutReasonTag = "Pick.SkillIdNone";
		return false;
	}

	const UJRPGSkillComponent *SC = Caster ?Caster->FindComponentByClass<UJRPGSkillComponent>() :nullptr;
	if (!SC||!SC->HasSkill(SkillId))
	{
		OutReasonTag ="Pick.SkillNotOwned";
		return false;
	}

	const UJRPGSkillDataAsset *SA = FindSkillAsset(Caster, SkillId);
	if (!SA)
	{
		OutReasonTag ="Pick.SkillAssetMissing";
		return false;
	}

	if (Settings.EligibilityPolicy== EChainEligibilityPolicy::ChainOnlyEligible&&!SA->bChainEligible)
	{
		OutReasonTag ="Pick.NotChainEligible";
		return false;
	}

	return true;
}

FJRPGOpResult UTrinityChainSubsystem::SubmitStepPick(AActor*Caster,FName SkillId)
{
	if (Ctx.State!= EChainState::SelectionFrozen)
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("Chain.NotInSelection"));

	FName Cant;
	if (!IsCasterPickable(Caster,Cant))
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make(Cant.ToString()));

	if (!Ctx.PrimaryTarget.IsValid())
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("Chain.PrimaryTargetInvalid"));

	FName SkillCant;
	if (!IsSkillEligibleForChain(Caster, SkillId, SkillCant))
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make(SkillCant.ToString()));

	// Toggle/Replace
	int32 Slot =INDEX_NONE;
	for (int32 i =0; i < Ctx.PartyActors.Num(); ++i)
	{
		if (Ctx.PartyActors[i].Get()==Caster)
		{
			Slot =i;
			break;
		}
	}
	if (Slot == INDEX_NONE)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Chain.CasterNotInParty"));

	FChainStepRuntime&Step = Ctx.Steps[Slot];

	if (Step.Pick.SkillId == SkillId)
	{
		// toggle clear
		Step.Pick.Caster = Caster;
		Step.Pick.SkillId = NAME_None;
		OnChainStepPickChanged.Broadcast(Caster, false, NAME_None);
		return FJRPGOpResult::Ok();
	}

	Step.Pick.Caster = Caster;
	Step.Pick.SkillId = SkillId;
	OnChainStepPickChanged.Broadcast(Caster, true, SkillId);
	return FJRPGOpResult::Ok();
}

FJRPGOpResult UTrinityChainSubsystem::ClearStepPick(AActor *Caster, FName /*ReasonTag*/)
{
	if (Ctx.State!= EChainState::SelectionFrozen)
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("Chain.NotInSelection"));

	for (FChainStepRuntime&Step : Ctx.Steps)
	{
		if (Step.Pick.Caster.Get() == Caster)
		{
			Step.Pick.SkillId = NAME_None;
			OnChainStepPickChanged.Broadcast(Caster, false, NAME_None);
			return FJRPGOpResult::Ok();
		}
	}
	return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Chain.NoPick"));
}

void UTrinityChainSubsystem::BuildExecuteQueueIfNeeded()
{
	// AutoFillMissingSteps
	if (!Settings.bAutoFillMissingSteps)return;

	for (int32 i =0; i < Ctx.Steps.Num(); ++i)
	{
		FChainStepRuntime &Step = Ctx.Steps[i];
		AActor *Caster = Ctx.PartyActors.IsValidIndex(i) ? Ctx.PartyActors[i].Get() : nullptr;
		if (!Caster) continue;

		Step.Pick.Caster = Caster;

		if (!Step.Pick.SkillId.IsNone()) continue;// already picked

		FName Cant;
		if (!IsCasterPickable(Caster,Cant))continue;

		AActor*Primary = Ctx.PrimaryTarget.Get();
		FName AutoSkill = NAME_None;

		// AI/AI-SP가 붙으면 여기서 “프리셋/롤/상황” 기반으로 추천하게 됨
		if (AutoPickSkillDelegate.IsBound())
		{
			AutoSkill =AutoPickSkillDelegate.Execute(Caster,Primary);
		}

		// fallback: 첫 번째 chain eligible
		if (AutoSkill.IsNone())
		{
			const UJRPGSkillComponent *SC = Caster->FindComponentByClass<UJRPGSkillComponent>();
			if (SC)
			{
				for (const auto &It :SC->SkillDB)
				{
					const UJRPGSkillDataAsset *SA = It.Value.Get();
					if (!SA) continue;
					
					if (Settings.EligibilityPolicy == EChainEligibilityPolicy::ChainOnlyEligible && !SA->bChainEligible)
						continue;
					AutoSkill = SA->SkillId;
					break;
				}
			}
		}

		if (!AutoSkill.IsNone())
		{
			FName SkillCant;
			if (IsSkillEligibleForChain(Caster,AutoSkill,SkillCant))
			{
				Step.Pick.SkillId = AutoSkill;
				OnChainStepPickChanged.Broadcast(Caster,true,AutoSkill);
			}
		}
	}
}

FJRPGOpResult UTrinityChainSubsystem::ConfirmSelection()
{
	if (Ctx.State!= EChainState::SelectionFrozen)
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("Chain.NotInSelection"));

	BuildExecuteQueueIfNeeded();

	TransitionTo(EChainState::ExecuteQueue);
	Ctx.StepIndex = 0;
	ExecuteNextStep();

	return FJRPGOpResult::Ok();
}

int32 UTrinityChainSubsystem::ComputeTPForSkill(constUSkillDataAsset*SkillAsset)const
{
	if (!SkillAsset) return 0;
	return FMath::Max(0,SkillAsset->ChainTPBase);
}

float UTrinityChainSubsystem::ComputeDamageScalarForStep() const
{
	float Scalar = 1.0f;
	if (Ctx.bChainStunBonusArmed)
	{
		Scalar *= Settings.ChainStunBonusDamageMultiplier;
	}
	return Scalar;
}

float UTrinityChainSubsystem::ComputeFinisherDamageScalar()const
{
	const float Scalar = 1.0f + (float)Ctx.TPTotal * Settings.FinisherDamageScalarPerTP;
	return FMath::Min(Scalar,Settings.FinisherDamageScalarMax);
}

void UTrinityChainSubsystem::ExecuteNextStep()
{
	if (Ctx.State != EChainState::ExecuteQueue)return;

	// abort safety
	if (!Ctx.PrimaryTarget.IsValid())
	{
		AbortChain("Chain.TargetInvalid");
		return;
	}

	if (Ctx.StepIndex >= 3)
	{
		ExecuteFinisher();
		return;
	}

	FChainStepRuntime &Step = Ctx.Steps[Ctx.StepIndex];
	AActor *Caster = Step.Pick.Caster.Get();
	const FName SkillId = Step.Pick.SkillId;

	// 미선택/미유효면 “스킵” (TP 0)
	if (!Caster||SkillId.IsNone())
	{
		Step.bExecuted = true;
		Step.bSucceeded = false;
		Step.FailReasonTag = "Step.Skipped";
		OnChainStepExecuted.Broadcast(Ctx.StepIndex,Caster,SkillId,false);

		Ctx.StepIndex++;
		ExecuteNextStep();
		return;
	}

	FName Cant;
	if (!IsCasterPickable(Caster,Cant))
	{
		Step.bExecuted = true;
		Step.bSucceeded = false;
		Step.FailReasonTag = Cant;
		OnChainStepExecuted.Broadcast(Ctx.StepIndex,Caster,SkillId,false);

		if (Settings.bAbortOnStepFail)
		{
			AbortChain("Chain.StepCasterInvalid");
			return;
		}

		Ctx.StepIndex++;
		ExecuteNextStep();
		return;
	}

	const UJRPGSkillComponent *SC_Const = Caster->FindComponentByClass<UJRPGSkillComponent>();
	UJRPGSkillComponent *SC = const_cast<UJRPGSkillComponent*>(SC_Const);
	if (!SC)
	{
		Step.bExecuted = true;
		Step.bSucceeded = false;
		Step.FailReasonTag ="Step.NoSkillComponent";
		OnChainStepExecuted.Broadcast(Ctx.StepIndex,Caster,SkillId,false);

		if (Settings.bAbortOnStepFail)
		{
			AbortChain("Chain.StepNoSkillComponent");
			return;
		}

		Ctx.StepIndex++;
		ExecuteNextStep();
		return;
	}

	// ---- ChainFreeExecution: AP/쿨다운 읽지도/쓰지도 않음 ----
	FJRPGSkillRequest Req;
	Req.SkillId = SkillId;
	Req.Instigator = Caster;
	Req.PrimaryTarget = Ctx.PrimaryTarget.Get();
	Req.SourceTag = "Chain";
	Req.bIgnoreCost = true;
	Req.bIgnoreCooldown = true;
	Req.bIgnoreGlobalCooldown = true;
	Req.ExecutionBudget = EJRPGExecutionBudget::ChainFree;// (아래 “필수 패치”에서 추가)
	Req.bRequireChainEligible = (Settings.EligibilityPolicy == EChainEligibilityPolicy::ChainOnlyEligible);
	Req.DamageScalar = ComputeDamageScalarForStep();

	const FJRPGSkillResult R = SC->RequestUseSkill(Req);

	Step.bExecuted = true;
	Step.bSucceeded = (R.Op.bOk&&R.bExecuted);

	if (Step.bSucceeded)
	{
		const UJRPGSkillDataAsset *SA = FindSkillAsset(Caster,SkillId);
		Step.TPGained = ComputeTPForSkill(SA);
		Ctx.TPTotal += Step.TPGained;
	}
	else
	{
		Step.FailReasonTag = R.Op.Reason.Tag;
	}

	OnChainStepExecuted.Broadcast(Ctx.StepIndex,Caster,SkillId,Step.bSucceeded);

	if (!Step.bSucceeded && Settings.bAbortOnStepFail)
	{
		AbortChain("Chain.StepFailed");
		return;
	}

	// 다음 스텝
	Ctx.StepIndex++;
	ExecuteNextStep();
}

void UTrinityChainSubsystem::ExecuteFinisher()
{
	TransitionTo(EChainState::Finisher);

	// Finisher caster: 파티 0번(컨트롤 캐릭터 체계 붙으면 그걸로 교체)
	AActor*FinisherCaster =Ctx.PartyActors.Num()>0 ?Ctx.PartyActors[0].Get() :nullptr;
	if (!FinisherCaster)
	{
		EndChain(false,"Chain.FinisherNoCaster");
		return;
	}

	UJRPGSkillComponent*SC =FinisherCaster->FindComponentByClass<UJRPGSkillComponent>();
	if (!SC)
	{
		EndChain(false,"Chain.FinisherNoSkillComponent");
		return;
	}

	FJRPGSkillRequest Req;
	Req.SkillId =Settings.FinisherSkillId;
	Req.Instigator =FinisherCaster;
	Req.PrimaryTarget =Ctx.PrimaryTarget.Get();
	Req.SourceTag ="ChainFinisher";
	Req.bIgnoreCost =true;
	Req.bIgnoreCooldown =true;
	Req.bIgnoreGlobalCooldown = true;
	Req.ExecutionBudget = EJRPGExecutionBudget::ChainFree;
	Req.bRequireChainEligible = false;// 피니셔는 별도 스킬일 수 있음
	Req.DamageScalar = ComputeFinisherDamageScalar();

	SC->RequestUseSkill(Req);

	EndChain(false,"Chain.End");
}

FJRPGOpResult UTrinityChainSubsystem::AbortChain(FName ReasonTag)
{
	if (!IsChainActive())
		return FJRPGOpResult::Ok();

	EndChain(true,ReasonTag.IsNone() ?"Chain.Abort" :ReasonTag);
	return FJRPGOpResult::Ok();
}

void UTrinityChainSubsystem::EndChain(bool bAborted,FName ReasonTag)
{
	TransitionTo(bAborted ? EChainState::Aborted : EChainState::Recover);

	// 정리 순서(핸들 누락 방지)
	ReleaseEntryGuards();

	// time handle release
	if (Ctx.TimeHandleValue != 0)
	{
		if (UCombatTimeSubsystem*Time =GetTime())
		{
			FCombatTimeHandle H;
			H.Value =Ctx.TimeHandleValue;
			Time->ReleaseTimeMode(H,"Chain.ReleaseTime");
		}
		Ctx.TimeHandleValue =0;
	}

	// SP reset (체인 종료 시 0) :contentReference[oaicite:5]{index=5}
	if (UCombatSynergyPointSubsystem *SP = GetSP())
	{
		SP->ResetForChainEnd();
	}

	OnChainEnded.Broadcast(Ctx.Token,bAborted,ReasonTag);

	// idle
	Ctx = FChainContext();
	Ctx.State = EChainState::Idle;
	Ctx.Steps.SetNum(3);

	TransitionTo(EChainState::Idle);
}

void UTrinityChainSubsystem::AutoConfirmIfDeadline(double Now)
{
	if (Ctx.State != EChainState::SelectionFrozen)return;
	if (Now >= Ctx.SelectionDeadlineReal)
	{
		ConfirmSelection();
	}
}

void UTrinityChainSubsystem::TickChainInsideGroggy()
{
	// 간단 판정: PrimaryTarget이 체인 시작 이후 “Stun 태그”를 얻으면 bonus armed
	// (프로젝트 Status/Groggy 시스템 확장 시 이벤트 구독 방식으로 교체 추천)
	if (!IsChainActive()) return;
	if (!Ctx.PrimaryTarget.IsValid()) return;
	if (Ctx.bChainStunBonusArmed) return;

	AActor *T = Ctx.PrimaryTarget.Get();

	const bool bStunned = (T->ActorHasTag("CC.Stun") || T->ActorHasTag("Groggy.Stunned") || T->ActorHasTag("State.Stunned"));
	if (bStunned)
	{
		Ctx.bChainStunBonusArmed =true;

		// 표식 Status는 Status 시스템이 소유(존재하면 적용)
		if (UActorComponent *Comp = T->GetComponentByClass(UActorComponent::StaticClass()))
		{
			// StatusComponent 구현이 붙으면 여기서 ApplyStatus 호출로 교체
			// 지금은 Tag 기반으로만 최소 보장
			T->Tags.AddUnique(Settings.ChainStunVulnerableStatusId);
		}
	}
}

void UTrinityChainSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld *W =GetWorld();
	if (!W)return;

	// 세션 종료/타겟 소멸 등 → Abort (중요)
	if (UCombatBattleSessionSubsystem *Session =GetSession())
	{
		if (!Session->IsCombatRunning() && IsChainActive())
		{
			AbortChain("Chain.Abort.SessionEnd");
			return;
		}
	}

	if (IsChainActive()&&!Ctx.PrimaryTarget.IsValid())
	{
		AbortChain("Chain.Abort.TargetGone");
		return;
	}

	const double Now = NowReal(W);

	// 전체 타임아웃(강제 중단 안전장치)
	if (IsChainActive())
	{
		const double Elapsed = Now - Ctx.StartTimeReal;
		if (Elapsed >= (double)Settings.ChainMaxRealSec)
		{
			AbortChain("Chain.Abort.Timeout");
			return;
		}
	}

	if (Ctx.State == EChainState::SelectionFrozen)
	{
		AutoConfirmIfDeadline(Now);

		// UI timer update (throttle ~20Hz)
		OnChainTimerUpdated.Broadcast(GetSelectionElapsedRealSec(),GetSelectionRemainingRealSec(),GetSelectionNormalized01(),true);
	}

	// Chain-inside groggy tracking (bonus armed)
	if (Ctx.State == EChainState::ExecuteQueue || Ctx.State == EChainState::Finisher)
	{
		TickChainInsideGroggy();
	}
}