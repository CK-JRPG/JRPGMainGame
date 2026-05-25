#include "Combat/AI/EnemyAIController.h"

#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/AI/CombatAIInterfaces.h"
#include "Combat/AI/CombatAIPresetAsset.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Presentation/TargetGuideLineComponent.h"
#include "Combat/Stats/HPComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledPawn = InPawn;
	ThreatComp = InPawn ? InPawn->FindComponentByClass<UThreatComponent>() : nullptr;
	SkillComp = InPawn ? InPawn->FindComponentByClass<USkillComponent>() : nullptr;
	CharComp = InPawn ? InPawn->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
	PresentationComp = InPawn ? InPawn->FindComponentByClass<UCombatPresentationComponent>() : nullptr;
	TargetGuideLineComp = InPawn ? InPawn->FindComponentByClass<UTargetGuideLineComponent>() : nullptr;

	LoadRangeParamsFromCharacterData();
	State = EEnemyCombatState::Engage;
}

void AEnemyAIController::LoadRangeParamsFromCharacterData()
{
	if (!CharComp) return;

	const UCombatCharacterDataAsset* Data = CharComp->GetCharacterData();
	if (!Data) return;

	bIsRanged = Data->bIsRangedCombatant;
	AttackRange = Data->AttackRange;
	PreferredMinRange = Data->PreferredMinRange;
	ChaseLeashRange = Data->ChaseLeashRange;
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ControlledPawn) return;

	if (const UHPComponent* SelfHP = ControlledPawn->FindComponentByClass<UHPComponent>(); SelfHP && SelfHP->IsDead())
	{
		StopMovement();
		SetCurrentTarget(nullptr);
		State = EEnemyCombatState::Idle;
		return;
	}

	RefreshStateFromGroggyAndChain();

	switch (State)
	{
	case EEnemyCombatState::Engage:
		RefreshTarget();
		if (CurrentTarget.IsValid())
		{
			const float Dist = GetDistanceToTarget();
			if (Dist <= AttackRange)
				State = EEnemyCombatState::Attack;
			else
				State = EEnemyCombatState::Chase;
		}
		break;

	case EEnemyCombatState::Chase:
		TickChase(DeltaSeconds);
		break;

	case EEnemyCombatState::Attack:
		TickAttack(DeltaSeconds);
		break;

	case EEnemyCombatState::Retreat:
		TickRetreat(DeltaSeconds);
		break;

	case EEnemyCombatState::Groggy_Stunned:
		TickGroggyStunned(DeltaSeconds);
		break;

	case EEnemyCombatState::Rising:
		TickRising(DeltaSeconds);
		break;

	case EEnemyCombatState::SuppressedByChain:
		return;

	default:
		break;
	}
}

void AEnemyAIController::ResetForNewBattle()
{
	StopMovement();
	SetCurrentTarget(nullptr);
	CachedChainProviderObject = nullptr;
	NextChainProviderRescanAt = 0.0f;
	TargetLockUntilReal = 0.0f;
	State = EEnemyCombatState::Engage;
}

void AEnemyAIController::RefreshStateFromGroggyAndChain()
{
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

	// Groggy/Chain 에서 복귀
	if (State == EEnemyCombatState::Groggy_Stunned
		|| State == EEnemyCombatState::Rising
		|| State == EEnemyCombatState::SuppressedByChain)
	{
		State = EEnemyCombatState::Engage;
	}
}

void AEnemyAIController::RefreshTarget()
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (ForcedTarget.IsValid() && Now < ForcedTargetUntilReal)
	{
		SetCurrentTarget(ForcedTarget.Get());
		return;
	}
	if (ForcedTarget.IsValid() && Now >= ForcedTargetUntilReal)
	{
		ForcedTarget = nullptr;
		ForcedTargetUntilReal = 0.0;
	}

	auto IsAliveTarget = [](AActor* InTarget) -> bool
		{
			if (!IsValid(InTarget))
			{
				return false;
			}
			const UHPComponent* HP = InTarget->FindComponentByClass<UHPComponent>();
			return !HP || !HP->IsDead();
		};

	if (CurrentTarget.IsValid() && !IsAliveTarget(CurrentTarget.Get()))
	{
		SetCurrentTarget(nullptr);
	}

	if (ThreatComp)
	{
		AActor* TopThreat = ThreatComp->GetTopThreatSource();
		if (IsAliveTarget(TopThreat))
		{
			SetCurrentTarget(TopThreat);
			return;
		}

	}

	UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
	if (!Battle || !ControlledPawn)
	{
		SetCurrentTarget(nullptr);
		return;
	}

	TArray<AActor*> Opponents;
	Battle->GetOpponentsFor(ControlledPawn.Get(), Opponents);

	float ClosestDistSq = FLT_MAX;
	AActor* Closest = nullptr;
	for (AActor* Opponent : Opponents)
	{
		if (!IsAliveTarget(Opponent))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(ControlledPawn->GetActorLocation(), Opponent->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Opponent;
		}
	}

	if (Closest)
	{
		SetCurrentTarget(Closest);
		return;
	}

	SetCurrentTarget(nullptr);
}

void AEnemyAIController::SetCurrentTarget(AActor* NewTarget)
{
	if (CurrentTarget.Get() != NewTarget)
	{
		CurrentTarget = NewTarget;
	}

	if (TargetGuideLineComp)
	{
		if (NewTarget)
		{
			TargetGuideLineComp->SetAggroTarget(NewTarget);
		}
		else
		{
			TargetGuideLineComp->ClearAggroTarget();
		}
	}
}

void AEnemyAIController::ForceSetCurrentTarget(AActor* NewTarget)
{
	SetCurrentTarget(NewTarget);
}

void AEnemyAIController::ApplyForcedTarget(AActor* NewTarget, float DurationSec)
{
	ForcedTarget = NewTarget;
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	ForcedTargetUntilReal = Now + FMath::Max(0.0, static_cast<double>(DurationSec));
	SetCurrentTarget(NewTarget);
}

bool AEnemyAIController::HasForcedTarget() const
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return ForcedTarget.IsValid() && Now < ForcedTargetUntilReal;
}

AActor* AEnemyAIController::GetEffectiveTargetActor() const
{
	if (HasForcedTarget())
	{
		return ForcedTarget.Get();
	}
	return CurrentTarget.Get();
}

void AEnemyAIController::TickChase(float DeltaSeconds)
{
	RefreshTarget();

	if (!CurrentTarget.IsValid())
	{
		State = EEnemyCombatState::Idle;
		return;
	}

	const float Dist = GetDistanceToTarget();

	if (Dist <= AttackRange)
	{
		State = EEnemyCombatState::Attack;
		return;
	}

	FaceTarget(CurrentTarget.Get());
	MoveDirectlyToward(CurrentTarget->GetActorLocation(), DeltaSeconds);
}

void AEnemyAIController::TickAttack(float DeltaSeconds)
{
	RefreshTarget();

	if (!CurrentTarget.IsValid())
	{
		State = EEnemyCombatState::Idle;
		return;
	}

	const float Dist = GetDistanceToTarget();

	// 원거리: 너무 가까우면 Retreat
	if (bIsRanged && PreferredMinRange > 0.f && Dist < PreferredMinRange)
	{
		State = EEnemyCombatState::Retreat;
		return;
	}

	// 사거리 밖이면 Chase
	if (bIsRanged && Dist > ChaseLeashRange)
	{
		State = EEnemyCombatState::Chase;
		return;
	}
	if (!bIsRanged && Dist > AttackRange)
	{
		State = EEnemyCombatState::Chase;
		return;
	}

	FaceTarget(CurrentTarget.Get());
	TryExecuteOffensiveAction(CurrentTarget.Get());
}

void AEnemyAIController::TickRetreat(float DeltaSeconds)
{
	if (!CurrentTarget.IsValid())
	{
		State = EEnemyCombatState::Idle;
		return;
	}

	const float Dist = GetDistanceToTarget();

	if (Dist >= PreferredMinRange)
	{
		State = EEnemyCombatState::Attack;
		return;
	}

	MoveDirectlyAwayFrom(CurrentTarget->GetActorLocation(), DeltaSeconds);
}

void AEnemyAIController::TickGroggyStunned(float DeltaSeconds)
{
	// 스턴 상태: 이동/공격 금지
}

void AEnemyAIController::TickRising(float DeltaSeconds)
{
	const bool bAllowed = PresetAsset ? PresetAsset->bEnemyRisingAttackAllowed : false;
	if (!bAllowed) return;

	const double Now = FPlatformTime::Seconds();
	if (Now >= TargetLockUntilReal)
	{
		const float Mult = PresetAsset ? PresetAsset->EnemyRisingTargetLockMultiplier : 2.0f;
		TargetLockUntilReal = Now + (1.0 * Mult);
		RefreshTarget();
	}

	if (CurrentTarget.IsValid() && PresentationComp) 
	{
		FaceTarget(CurrentTarget.Get());
		TryExecuteOffensiveAction(CurrentTarget.Get());
	}
}

void AEnemyAIController::TryExecuteOffensiveAction(AActor* Target)
{
	if (!PresentationComp || !IsValid(Target))
	{
		return;
	}

	// TODO: 몬스터 스킬 우선순위(쿨다운/거리/상태이상)를 추가할 수 있도록 진입점 분리.
	PresentationComp->TryPresentBasicAttack(Target);
}

// ---- NavMesh 미사용 직접 이동 ----

void AEnemyAIController::MoveDirectlyToward(const FVector& Destination, float DeltaTime)
{
	ACharacter* MyChar = Cast<ACharacter>(ControlledPawn.Get());
	if (!MyChar) return;

	FVector Dir = Destination - MyChar->GetActorLocation();
	Dir.Z = 0.f;

	const float Dist = Dir.Size();
	if (Dist < 10.0f) return;

	Dir /= Dist;
	MyChar->AddMovementInput(Dir, 1.0f);
}

void AEnemyAIController::MoveDirectlyAwayFrom(const FVector& ThreatLocation, float DeltaTime)
{
	ACharacter* MyChar = Cast<ACharacter>(ControlledPawn.Get());
	if (!MyChar) return;

	FVector Dir = MyChar->GetActorLocation() - ThreatLocation;
	Dir.Z = 0.f;

	const float Dist = Dir.Size();
	if (Dist < 1.0f)
	{
		Dir = MyChar->GetActorForwardVector();
	}
	else
	{
		Dir /= Dist;
	}

	MyChar->AddMovementInput(Dir, 1.0f);
}

void AEnemyAIController::FaceTarget(AActor* Target)
{
	if (!Target || !ControlledPawn) return;

	FVector Dir = Target->GetActorLocation() - ControlledPawn->GetActorLocation();
	Dir.Z = 0.f;
	if (!Dir.IsNearlyZero())
	{
		ControlledPawn->SetActorRotation(Dir.Rotation());
	}
}

float AEnemyAIController::GetDistanceToTarget() const
{
	if (!CurrentTarget.IsValid() || !ControlledPawn) return MAX_FLT;
	return FVector::Dist2D(ControlledPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
}

// ---- 유틸리티 ----


bool AEnemyAIController::IsChainSequenceActive() const
{
	if (!GetWorld()) return false;
 
	if (UObject* CachedProvider = CachedChainProviderObject.Get())
	{
		if (ICombatChainFlowProvider* Chain = Cast<ICombatChainFlowProvider>(CachedProvider))
		{
			return Chain->IsChainSequenceActive();
		}
		CachedChainProviderObject = nullptr;
	}

	const double Now = FPlatformTime::Seconds();
	if (Now < NextChainProviderRescanAt)
	{
		return false;
	}
	NextChainProviderRescanAt = Now + ChainProviderRescanIntervalSec;

	for (TObjectIterator<UObject> It; It; ++It)
	{
		UObject* Obj = *It;
		if (!Obj || Obj->GetWorld() != GetWorld()) continue;
 
		if (Obj->GetClass()->ImplementsInterface(UCombatChainFlowProvider::StaticClass()))
		{
			ICombatChainFlowProvider* Chain = Cast<ICombatChainFlowProvider>(Obj);
			if (Chain && Chain->IsChainSequenceActive())
			{
				CachedChainProviderObject = Obj;
				return true;
			}
		}
	}
	return false;
} 

bool AEnemyAIController::ReadGroggy(EJRPGGroggyPhase& OutPhase) const
{
	OutPhase = EJRPGGroggyPhase::Normal;
	if (!ControlledPawn) return false;

	TArray<UActorComponent*> Comps;
	ControlledPawn->GetComponents(Comps);

	for (UActorComponent* C : Comps)
	{
		if (C && C->GetClass()->ImplementsInterface(UCombatGroggyProvider::StaticClass()))
		{
			ICombatGroggyProvider* G = Cast<ICombatGroggyProvider>(C);
			if (G)
			{
				OutPhase = G->GetGroggyPhase();
				return true;
			}
		}
	}
	return false;
}
