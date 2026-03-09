// Source/JRPGCombat/Private/Combat/AI/EnemyAIController.cpp

#include "Combat/AI/EnemyAIController.h"

#include "JRPGCombat/Public/Combat/Threat/CombatThreatComponent.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/AI/CombatAIInterfaces.h"

#include "GameFramework/Pawn.h"
#include "Engine/World.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemyAIController::OnPossess(APawn *InPawn)
{
	Super::OnPossess(InPawn);

	ControlledPawn = InPawn;
	ThreatComp = InPawn ? InPawn->FindComponentByClass<UCombatThreatComponent>() : nullptr;
	SkillComp  = InPawn ? InPawn->FindComponentByClass<USkillComponent>() : nullptr;

	State = EEnemyCombatState::Engage;
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ControlledPawn)
		return;

	RefreshStateFromGroggyAndChain();

	switch (State)
	{
	case EEnemyCombatState::Combat_Normal:
		TickCombatNormal(DeltaSeconds);
		break;
	case EEnemyCombatState::Groggy_Stunned:
		TickGroggyStunned(DeltaSeconds);
		break;
	case EEnemyCombatState::Rising:
		TickRising(DeltaSeconds);
		break;
	case EEnemyCombatState::SuppressedByChain:
		// 체인 시퀀스 중에는 적 AI 완전 억제(별도 전투처럼 보이게)
		return;
	default:
		break;
	}
}

void AEnemyAIController::RefreshStateFromGroggyAndChain()
{
	// Chain Active면 무조건 억제
	if (IsChainSequenceActive())
	{
		State = EEnemyCombatState::SuppressedByChain;
		return;
	}

	EJRPGGroggyPhase Phase = EJRPGGroggyPhase::Normal;
	if (ReadGroggy(Phase))
	{
		if (Phase == EJRPGGroggyPhase::Stunned)
		{
			State = EEnemyCombatState::Groggy_Stunned;
			return;
		}
		if (Phase == EJRPGGroggyPhase::Rising)
		{
			State = EEnemyCombatState::Rising;
			return;
		}
	}

	// 기본 전투
	if (State != EEnemyCombatState::Combat_Normal)
		State = EEnemyCombatState::Combat_Normal;
}

void AEnemyAIController::TickCombatNormal(float DeltaSeconds)
{
	if (!ThreatComp || !SkillComp)
		return;

	// Threat 기반 타겟 :contentReference[oaicite:41]{index=41}
	AActor *Target = ThreatComp->GetCurrentTarget();
	if (!IsValid(Target))
		return;

	// 간단: 기본 공격 or 패턴 스킬 (패턴은 너희 SkillPatternTableId로 확장 가능 :contentReference[oaicite:42]{index=42})
	if (SkillComp->CanUseBasicAttack())
	{
		SkillComp->RequestBasicAttack(Target);
	}
}

void AEnemyAIController::TickGroggyStunned(float DeltaSeconds)
{
	// 문서: 스턴(그로기) 상태에서 공격이 완전히 멈춘다 :contentReference[oaicite:43]{index=43}
	// 공격/스킬/이동 행동 금지 :contentReference[oaicite:44]{index=44}
	StopMovement();
}

void AEnemyAIController::TickRising(float DeltaSeconds)
{
	// 문서: Rising 동안 기본 공격 금지(또는 매우 제한) :contentReference[oaicite:45]{index=45}
	// 옵션: 데이터로 허용 가능 :contentReference[oaicite:46]{index=46}
	const bool bAllowed = PresetAsset ? PresetAsset->bEnemyRisingAttackAllowed : false;
	if (!bAllowed)
	{
		StopMovement();
		return;
	}

	// 허용 시에도 타겟 전환을 최소화(락 시간 증가)
	const double Now = FPlatformTime::Seconds();
	if (Now < TargetLockUntilReal)
	{
		// 락 중: 기존 타겟 유지
	}
	else
	{
		const float Mult =PresetAsset ? PresetAsset->EnemyRisingTargetLockMultiplier :2.0f;
		TargetLockUntilReal = Now+ (1.0 * Mult);
	}

	if (ThreatComp && SkillComp)
	{
		if (AActor *Target = ThreatComp->GetCurrentTarget())
		{
			if (SkillComp->CanUseBasicAttack())
				SkillComp->RequestBasicAttack(Target);
		}
	}
}

bool AEnemyAIController::IsChainSequenceActive()const
{
	if (!GetWorld())
		return false;

	// World 안에서 ICombatChainFlowProvider 구현 객체를 찾는다(TrinityChainSubsystem에서 구현하도록 연결)
	for (TObjectIterator<UObject> It; It; ++It)
	{
		UObject *Obj = *It;
		if (!Obj || Obj->GetWorld() != GetWorld())
			continue;

		if (Obj->GetClass()->ImplementsInterface(UCombatChainFlowProvider::StaticClass()))
		{
			ICombatChainFlowProvider *Chain = Cast<ICombatChainFlowProvider>(Obj);
			if (Chain && Chain->IsChainSequenceActive())
				return true;
		}
	}
	return false;
}

bool AEnemyAIController::ReadGroggy(EJRPGGroggyPhase &OutPhase)const
{
	OutPhase = EJRPGGroggyPhase::Normal;

	if (!ControlledPawn)
		return false;

	TArray<UActorComponent*> Comps;
	ControlledPawn->GetComponents(Comps);

	for (UActorComponent *C : Comps)
	{
		if (C && C->GetClass()->ImplementsInterface(UCombatGroggyProvider::StaticClass()))
		{
			ICombatGroggyProvider *G = Cast<ICombatGroggyProvider>(C);
			if (G)
			{
				OutPhase = G->GetGroggyPhase();
				return true;
			}
		}
	}
	return false;
}