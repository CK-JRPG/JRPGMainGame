#include "Combat/AI/EnemyAIController.h"

#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/AI/CombatAIInterfaces.h"
#include "Combat/AI/CombatAIPresetAsset.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "Combat/Presentation/TargetGuideLineComponent.h"
#include "Combat/Stats/HPComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

namespace
{
	bool IsAliveCombatActor(AActor* InActor)
	{
		if (!IsValid(InActor))
		{
			return false;
		}

		const UHPComponent* HP = InActor->FindComponentByClass<UHPComponent>();
		return !HP || !HP->IsDead();
	}
}

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
	AttackStartRange = bIsRanged ? FMath::Max(AttackRange, 600.f) : FMath::Clamp(AttackRange > 0.f ? AttackRange : 320.f, 250.f, 350.f);
	AttackKeepRange = bIsRanged ? FMath::Max(AttackStartRange + 150.f, 750.f) : FMath::Max(AttackStartRange + 80.f, 430.f);
	ChaseLeashRange = FMath::Clamp(ChaseLeashRange > 0.f ? ChaseLeashRange : 1000.f, 800.f, 1200.f);
	LeashRange = FMath::Max(1500.f, ChaseLeashRange + 300.f);
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
			if (Dist <= AttackStartRange)
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
	TargetSwitchLockedUntilReal = 0.0;
	NextAttackAllowedReal = 0.0;
	WindupUntilReal = 0.0;
	RecoveryUntilReal = 0.0;
	WindupTarget = nullptr;
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

bool AEnemyAIController::IsTargetOverFocusLimit(AActor* Candidate, const UBattleSessionSubsystem* Battle) const
{
	if (!IsAliveCombatActor(Candidate) || !Battle || !ControlledPawn)
	{
		return false;
	}

	TArray<AActor*> Opponents;
	Battle->GetOpponentsFor(ControlledPawn.Get(), Opponents);
	int32 AliveAlternativeTargets = 0;
	for (AActor* Opponent : Opponents)
	{
		if (Opponent != Candidate && IsAliveCombatActor(Opponent))
		{
			++AliveAlternativeTargets;
		}
	}
	if (AliveAlternativeTargets <= 0)
	{
		return false;
	}

	TArray<AActor*> EnemyActors;
	Battle->GetAlliesFor(ControlledPawn.Get(), EnemyActors);
	EnemyActors.Add(ControlledPawn.Get());

	const uint32 SelfId = ControlledPawn->GetUniqueID();
	int32 AliveEnemyCount = 0;
	int32 CandidateClaimCount = 0;
	int32 LowerIdCandidateClaimCount = 0;
	bool bSelfClaimsCandidate = false;

	for (AActor* EnemyActor : EnemyActors)
	{
		if (!IsAliveCombatActor(EnemyActor))
		{
			continue;
		}

		++AliveEnemyCount;

		const APawn* EnemyPawn = Cast<APawn>(EnemyActor);
		const AEnemyAIController* EnemyController = EnemyPawn ? Cast<AEnemyAIController>(EnemyPawn->GetController()) : nullptr;
		if (!EnemyController || EnemyController->GetEffectiveTargetActor() != Candidate)
		{
			continue;
		}

		++CandidateClaimCount;
		if (EnemyActor == ControlledPawn.Get())
		{
			bSelfClaimsCandidate = true;
		}
		else if (EnemyActor->GetUniqueID() < SelfId)
		{
			++LowerIdCandidateClaimCount;
		}
	}

	if (AliveEnemyCount <= 1)
	{
		return false;
	}

	const int32 MaxClaims = FMath::Clamp(
		FMath::FloorToInt(static_cast<float>(AliveEnemyCount) * MaxFocusedTargetShare + 0.001f),
		1,
		AliveEnemyCount);

	if (bSelfClaimsCandidate)
	{
		if (CandidateClaimCount <= MaxClaims)
		{
			return false;
		}

		const int32 SelfClaimRank = LowerIdCandidateClaimCount + 1;
		return SelfClaimRank > MaxClaims;
	}

	return CandidateClaimCount >= MaxClaims;
}

AActor* AEnemyAIController::SelectBestThreatTargetRespectingFocus(const TArray<AActor*>& Candidates, const UBattleSessionSubsystem* Battle) const
{
	if (!ThreatComp)
	{
		return nullptr;
	}

	float BestThreat = 0.f;
	AActor* BestTarget = nullptr;
	for (AActor* Candidate : Candidates)
	{
		if (!IsAliveCombatActor(Candidate) || IsTargetOverFocusLimit(Candidate, Battle))
		{
			continue;
		}

		const float Threat = ThreatComp->GetThreat(Candidate);
		if (Threat > BestThreat)
		{
			BestThreat = Threat;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

AActor* AEnemyAIController::SelectClosestTargetRespectingFocus(const TArray<AActor*>& Candidates, const UBattleSessionSubsystem* Battle) const
{
	if (!ControlledPawn)
	{
		return nullptr;
	}

	float ClosestDistSq = FLT_MAX;
	AActor* Closest = nullptr;
	for (AActor* Candidate : Candidates)
	{
		if (!IsAliveCombatActor(Candidate) || IsTargetOverFocusLimit(Candidate, Battle))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(ControlledPawn->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Candidate;
		}
	}

	return Closest;
}

void AEnemyAIController::RefreshTarget()
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (ForcedTarget.IsValid() && Now < ForcedTargetUntilReal)
	{
		SetCurrentTarget(ForcedTarget.Get(), false);
		return;
	}
	if (ForcedTarget.IsValid() && Now >= ForcedTargetUntilReal)
	{
		ForcedTarget = nullptr;
		ForcedTargetUntilReal = 0.0;
	}

	auto IsAliveTarget = [](AActor* InTarget) -> bool
		{
			return IsAliveCombatActor(InTarget);
		};

	if (CurrentTarget.IsValid() && !IsAliveTarget(CurrentTarget.Get()))
	{
		SetCurrentTarget(nullptr, false);
	}

	const bool bCurrentTargetAlive = CurrentTarget.IsValid() && IsAliveTarget(CurrentTarget.Get());
	UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
	if (!Battle || !ControlledPawn)
	{
		SetCurrentTarget(nullptr, false);
		return;
	}

	TArray<AActor*> Opponents;
	Battle->GetOpponentsFor(ControlledPawn.Get(), Opponents);
	const bool bCurrentTargetOverFocused = bCurrentTargetAlive && IsTargetOverFocusLimit(CurrentTarget.Get(), Battle);

	if (ThreatComp)
	{
		AActor* TopThreat = ThreatComp->GetTopThreatSource();
		AActor* ThreatTarget = SelectBestThreatTargetRespectingFocus(Opponents, Battle);
		if (!ThreatTarget && IsAliveTarget(TopThreat))
		{
			ThreatTarget = SelectClosestTargetRespectingFocus(Opponents, Battle);
			if (!ThreatTarget)
			{
				ThreatTarget = TopThreat;
			}
		}

		if (IsAliveTarget(ThreatTarget))
		{
			if (bCurrentTargetAlive && ThreatTarget != CurrentTarget.Get() && !bCurrentTargetOverFocused)
			{
				if (Now < TargetSwitchLockedUntilReal)
				{
					return;
				}

				const float CurrentThreat = ThreatComp->GetThreat(CurrentTarget.Get());
				const float TopThreatValue = ThreatComp->GetThreat(ThreatTarget);
				if (CurrentThreat > 0.f && TopThreatValue < CurrentThreat * TargetSwitchThreatRatio)
				{
					return;
				}
			}

			SetCurrentTarget(ThreatTarget);
			return;
		}

	}

	if (bCurrentTargetAlive && Now < TargetSwitchLockedUntilReal && !bCurrentTargetOverFocused)
	{
		return;
	}

	float ClosestDistSq = FLT_MAX;
	AActor* Closest = SelectClosestTargetRespectingFocus(Opponents, Battle);
	if (!Closest)
	{
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
	}

	if (Closest)
	{
		SetCurrentTarget(Closest, false);
		return;
	}

	SetCurrentTarget(nullptr, false);
}

void AEnemyAIController::SetCurrentTarget(AActor* NewTarget, bool bApplySwitchLock)
{
	if (CurrentTarget.Get() != NewTarget)
	{
		CurrentTarget = NewTarget;
		if (bApplySwitchLock && NewTarget)
		{
			const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
			TargetSwitchLockedUntilReal = Now + FMath::Max(0.f, TargetSwitchLockSec);
		}
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
	SetCurrentTarget(NewTarget, false);
}

void AEnemyAIController::ApplyForcedTarget(AActor* NewTarget, float DurationSec)
{
	ForcedTarget = NewTarget;
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	ForcedTargetUntilReal = Now + FMath::Max(0.0, static_cast<double>(DurationSec));
	TargetSwitchLockedUntilReal = 0.0;
	SetCurrentTarget(NewTarget, false);
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

	if (Dist > LeashRange)
	{
		StopMovement();
		SetCurrentTarget(nullptr, false);
		State = EEnemyCombatState::Engage;
		return;
	}

	if (Dist <= AttackStartRange)
	{
		State = EEnemyCombatState::Attack;
		return;
	}

	FaceTarget(CurrentTarget.Get());
	MoveDirectlyToward(CurrentTarget->GetActorLocation(), DeltaSeconds);
}

void AEnemyAIController::TickAttack(float DeltaSeconds)
{
	if (PresentationComp && PresentationComp->HasActivePresentation())
	{
		return;
	}

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

	if (Dist > LeashRange)
	{
		StopMovement();
		SetCurrentTarget(nullptr, false);
		State = EEnemyCombatState::Engage;
		return;
	}

	// 공격 유지 사거리 밖이면 다시 접근합니다.
	if (Dist > AttackKeepRange || (bIsRanged && Dist > ChaseLeashRange))
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

	if (PresentationComp && PresentationComp->HasActivePresentation())
	{
		return;
	}

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
	if (!PresentationComp || !IsValid(Target) || !ControlledPawn)
	{
		return;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (Now < RecoveryUntilReal || Now < NextAttackAllowedReal || PresentationComp->HasActivePresentation())
	{
		return;
	}

	if (WindupUntilReal <= 0.0 || WindupTarget.Get() != Target)
	{
		WindupTarget = Target;
		WindupUntilReal = Now + FMath::FRandRange(0.4f, 0.7f);
		StopMovement();
		UE_LOG(LogTemp, Log, TEXT("[EnemyTempo] Windup Owner=%s Target=%s Duration=%.2f"), *GetNameSafe(ControlledPawn.Get()), *GetNameSafe(Target), WindupUntilReal - Now);
		return;
	}

	if (Now < WindupUntilReal)
	{
		StopMovement();
		FaceTarget(Target);
		return;
	}

	const float Dist = GetDistanceToTarget();
	if (Dist > AttackKeepRange)
	{
		WindupUntilReal = 0.0;
		WindupTarget = nullptr;
		State = EEnemyCombatState::Chase;
		return;
	}

	const FCombatActionResult Result = PresentationComp->TryPresentBasicAttack(Target);
	if (Result.bOk)
	{
		const float RecoverySec = FMath::FRandRange(0.6f, 1.0f);
		const float CooldownSec = FMath::FRandRange(0.5f, 1.2f);
		RecoveryUntilReal = Now + RecoverySec;
		NextAttackAllowedReal = Now + FMath::Clamp(RecoverySec + CooldownSec, 1.8f, 2.5f);
		UE_LOG(LogTemp, Log, TEXT("[EnemyTempo] Attack Owner=%s Target=%s Recovery=%.2f CooldownReadyIn=%.2f"), *GetNameSafe(ControlledPawn.Get()), *GetNameSafe(Target), RecoverySec, NextAttackAllowedReal - Now);
	}
	else
	{
		NextAttackAllowedReal = Now + 0.2;
	}

	WindupUntilReal = 0.0;
	WindupTarget = nullptr;
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
