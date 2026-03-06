// Source/JRPGCombat/Private/Combat/AI/EnemyAIController.cpp
#include "Combat/AI/EnemyAIController.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"

#include "Combat/AI/CombatAIContext.h"
#include "Combat/AI/CombatAIScorer.h"
#include "Combat/AI/CombatAIPresetAsset.h"

#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Stats/HPComponent.h"

#include "Combat/Infrastructure/BattleSessionSubsystem.h"
#include "Combat/Infrastructure/TrinityChainSubsystem.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Status/StatusComponent.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyAIController::OnPossess(APawn*InPawn)
{
	Super::OnPossess(InPawn);

	// 캐시
	if (InPawn)
	{
		CachedThreat = InPawn->FindComponentByClass<UThreatComponent>();
	}

	StartLoop();
}

void AEnemyAIController::OnUnPossess()
{
	StopLoop();
	CachedThreat = nullptr;
	Super::OnUnPossess();
}

void AEnemyAIController::StartLoop()
{
	if (!GetWorld()||!PresetAsset)return;

	const float Interval = FMath::Max(0.05f, PresetAsset->Enemy.DecisionIntervalRealSec);
	GetWorld()->GetTimerManager().SetTimer(DecisionTimer, this,  &AEnemyAIController::ThinkOnce, Interval, true);
}

void AEnemyAIController::StopLoop()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(DecisionTimer);
}

float AEnemyAIController::NowReal()const
{
	return GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
}

bool AEnemyAIController::IsSuppressedByChain()const
{
	if (!GetWorld()) return false;
	if (UTrinityChainSubsystem *Chain = GetWorld()->GetSubsystem<UTrinityChainSubsystem>())
	{
		return Chain->IsEnemySuppressed();
	}
	return false;
}

void AEnemyAIController::ApplySuppressedStop()
{
	StopMovement();

	APawn *P = GetPawn();
	if (!P)return;

	// 스킬/공격 중단(스킬 시스템에 취소 API가 있다면 여기서 호출)
	if (USkillComponent *Skill = P->FindComponentByClass<USkillComponent>())
	{
		Skill->CancelAllActions(TEXT("ChainSuppressed"));
	}
}

EEnemyCombatAIState AEnemyAIController::ComputeState()const
{
	APawn*P = GetPawn();
	if (!P) return EEnemyCombatAIState::Idle;

	if (!GetWorld()) return EEnemyCombatAIState::Idle;

	UBattleSessionSubsystem *Session = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	if (!Session || !Session->IsSessionActive())
	{
		return EEnemyCombatAIState::Idle;
	}

	if (IsSuppressedByChain())
	{
		return EEnemyCombatAIState::Suppressed;
	}

	// CC면 행동불가
	if (UStatusComponent *Status = P->FindComponentByClass<UStatusComponent>())
	{
		if (Status->IsActionBlockedByCC())
		{
			// CC 자체를 상태로 분리해도 되지만, 여기선 “행동 금지” 처리로 충분
			return EEnemyCombatAIState::Combat_Normal;
		}
	}

	// 그로기 상태
	if (UGroggyComponent *Groggy =P->FindComponentByClass<UGroggyComponent>())
	{
		if (Groggy->GetPhase() == EGroggyPhase::Stunned)
		{
			return EEnemyCombatAIState::Groggy_Stunned;
		}
		if (Groggy->GetPhase() == EGroggyPhase::Rising)
		{
			return EEnemyCombatAIState::Rising;
		}
	}

	return EEnemyCombatAIState::Combat_Normal;
}

void AEnemyAIController::RefreshStateFromSystems()
{
	const EEnemyCombatAIState NewState =ComputeState();
	if (NewState != State)
	{
		State = NewState;

		// 체인 억제 진입 시 즉시 멈춤
		if (State == EEnemyCombatAIState::Suppressed)
		{
			ApplySuppressedStop();
		}
	}
}

void AEnemyAIController::EnsureEngageInitialThreat()
{
	if (!GetWorld() || !GetPawn())return;

	UBattleSessionSubsystem *Session = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	if (!Session || !Session->IsSessionActive())return;

	UThreatComponent *Threat = CachedThreat.Get();
	if (!Threat) return;

	// 전투 시작 시 “조작 중이던 플레이어”를 기본 타겟으로 잡고 초기 Threat 부여 (Aggro 문서)
	AActor *PlayerControlled = Session->GetPlayerControlledPartyMember();
	if (!PlayerControlled) return;

	// 이미 타겟이 있으면 중복 Engage 방지
	if (Threat->GetCurrentTarget() == nullptr)
	{
		Threat->AddInitialThreat(PlayerControlled, PresetAsset ? PresetAsset->Enemy.InitialThreatOnEngage : 50.f, TEXT("EngageInit"));
		Threat->TrySwitchTarget();// 즉시 타겟 확정
	}
}

AActor* AEnemyAIController::ResolveCurrentTarget()
{
	UThreatComponent *Threat = CachedThreat.Get();
	if (!Threat)return nullptr;

	// Threat 시스템이 권위: AI는 조회만
	AActor*T = Threat->GetCurrentTarget();
	if (!IsValid(T))
	{
		// 사망/무효면 Threat 쪽에서 재계산될 것이고, 여기선 TrySwitchTarget로 유도
		Threat->TrySwitchTarget();
		T = Threat->GetCurrentTarget();
	}
	return T;
}

void AEnemyAIController::ThinkOnce()
{
	if (!GetPawn() || !PresetAsset) return;

	RefreshStateFromSystems();

	// 세션 비활성/사망은 즉시 대기
	UCombatAIContext *CtxObj = UCombatAIContext::Build(GetPawn());
	if (!CtxObj || !CtxObj->bSessionActive || !CtxObj->IsAlive())
	{
		return;
	}

	// 체인 억제면 행동 금지(제노블3처럼 “체인=별도 전투 시퀀스”)
	if (State == EEnemyCombatAIState::Suppressed)
	{
		return;
	}

	// 그로기 스턴이면 행동 금지
	if (State == EEnemyCombatAIState::Groggy_Stunned)
	{
		return;
	}

	// 라이징 공격 제한
	if (State == EEnemyCombatAIState::Rising && !PresetAsset->Enemy.bRisingAttackAllowed)
	{
		return;
	}

	// Engage 초기 Threat 보장
	EnsureEngageInitialThreat();

	// 타겟/스위칭은 ThreatComponent의 규칙에 따름
	if (UThreatComponent *Threat =CachedThreat.Get())
	{
		Threat->TrySwitchTarget();// Threat 문서: AI가 일정 주기로 호출 가능
	}

	AActor *Target = ResolveCurrentTarget();
	if (!Target) return;

	// 결정
	const FCombatAIAction Action = FCombatAIScorer::ChooseEnemyAction(*CtxObj, *PresetAsset, State, Target);

	USkillComponent *Skill = GetPawn()->FindComponentByClass<USkillComponent>();
	if (!Skill) return;

	// 내부 쿨(적 AP/쿨이 없을 수 있으므로)
	const float Now = NowReal();

	auto IsInternalReady = [&](FName SkillId)->bool
	{
		if (SkillId.IsNone())return false;
		if (const float*Next =SkillNextAvailableReal.Find(SkillId))
		{
			return Now>= *Next;
		}
		return true;
	};

	auto CommitInternalCooldown = [&](FName SkillId)
	{
		if (!SkillId.IsNone())
		{
			// 패턴에서 해당 스킬의 내부 쿨을 찾고, 없으면 기본 3초
			float CD = 3.0f;
			for (const FEnemySkillPatternEntry &E : PresetAsset->Enemy.Pattern)
			{
				if (E.SkillId == SkillId)
				{
					CD = FMath::Max(0.1f, E.InternalCooldownRealSec);
					break;
				}
			}
			SkillNextAvailableReal.Add(SkillId,Now + CD);
		}
	};

	switch (Action.Type)
	{
	case ECombatAIActionType::UseSkill:
		{
			if (!IsInternalReady(Action.SkillId))
			{
				// 내부쿨이면 기본공격으로 폴백
				Skill->RequestBasicAttack(Target, ESkillRequestSource::AI);
				return;
			}

			const bool bOk = Skill->RequestUseSkill(Action.SkillId,Target, ESkillRequestSource::AI);
			if (bOk)
			{
				CommitInternalCooldown(Action.SkillId);
			}
			else
			{
				// 실패 시 기본공격(문서: 실패 Reason은 Skill이 가진다)
				Skill->RequestBasicAttack(Target, ESkillRequestSource::AI);
			}
			break;
		}
	case ECombatAIActionType::BasicAttack:
		{
			Skill->RequestBasicAttack(Target, ESkillRequestSource::AI);
			break;
		}
	default:
		break;
	}
}