#include "Combat/AI/CombatPartyAIComponent.h"
#include "Combat/AI/CombatAIContext.h"
#include "Combat/AI/CombatAIScorer.h"

#include "Combat/Skills/SkillComponent.h"
#include "Combat/Skills/SkillDataAsset.h"
#include "Combat/Skills/SkillTypes.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Characters/CombatParticipantInterface.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/AI/EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"

UCombatPartyAIComponent::UCombatPartyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
}

void UCombatPartyAIComponent::BeginPlay()
{
	Super::BeginPlay();

	Context = NewObject<UCombatAIContext>(this);
	Context->Initialize(GetOwner(), Role, PresetAsset);

	Scorer = NewObject<UCombatAIScorer>(this);
	Scorer->Initialize(FGetSkillAIMetaDelegate::CreateUObject(
		this, &UCombatPartyAIComponent::ResolveSkillMeta
	));

	CachedPresentation = GetOwner() ? GetOwner()->FindComponentByClass<UCombatPresentationComponent>() : nullptr;
	RangedRepositionDirection = FMath::RandBool() ? 1.f : -1.f;

	LoadRangeParams();
	NavFailureRetryBlockRemaining = 0.f;
    NavFailureLogCooldownRemaining = 0.f;

	UE_LOG(LogTemp, Log, TEXT("[RoleDebug] %s RoleType=%s"), *GetNameSafe(GetOwner()), *RoleToDebugString(Role));

	if (UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
	{
		TArray<AActor*> Allies;
		Battle->GetAlliesFor(GetOwner(), Allies);
		for (AActor* Ally : Allies)
		{
			if (!IsValid(Ally)) continue;
			if (UCombatPartyAIComponent* AllyAI = Ally->FindComponentByClass<UCombatPartyAIComponent>())
			{
				UE_LOG(LogTemp, Log, TEXT("[RoleDebug] %s RoleType=%s"), *GetNameSafe(Ally), *RoleToDebugString(AllyAI->Role));
			}
		}
	}
}

void UCombatPartyAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UCombatPartyAIComponent::LoadRangeParams()
{
	if (!GetOwner()) return;

	UCombatCharacterComponent* CharComp = GetOwner()->FindComponentByClass<UCombatCharacterComponent>();
	if (!CharComp) return;

	const UCombatCharacterDataAsset* Data = CharComp->GetCharacterData();
	if (!Data) return;

	bIsRanged = Data->bIsRangedCombatant;
	AttackRange = Data->AttackRange;
	PreferredMinRange = Data->PreferredMinRange;
	ChaseLeashRange = Data->ChaseLeashRange;

	// Supporter는 기본적으로 원거리 -- 데이터에 설정이 없으면 안전한 기본값 적용
	constexpr float DefaultSupporterAttackRange = 600.f;
	constexpr float DefaultSupporterMinRange = 300.f;
	if (Role == EJRPGPartyRole::Supporter && !bIsRanged)
	{
		bIsRanged = true;
		if (AttackRange < DefaultSupporterAttackRange) AttackRange = DefaultSupporterAttackRange;
		if (PreferredMinRange < DefaultSupporterMinRange) PreferredMinRange = DefaultSupporterMinRange;
	}
}

void UCombatPartyAIComponent::NotifyDamagedBy(AActor* Attacker)
{
	if (IsValid(Attacker))
	{
		LastAttacker = Attacker;
	}
}

void UCombatPartyAIComponent::SetCurrentTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
}

void UCombatPartyAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Context || !Scorer) return;
	NavFailureRetryBlockRemaining = FMath::Max(0.f, NavFailureRetryBlockRemaining - DeltaTime);
	NavFailureLogCooldownRemaining = FMath::Max(0.f, NavFailureLogCooldownRemaining - DeltaTime);
 
	if (IsPlayerControlledNow())
	{
		return;
	}

	if (Context->bSelfIsDead)
	{
		return;
	}

	RefreshContext();
	if (Role == EJRPGPartyRole::Defender)
	{
		TankTickLogAccum += DeltaTime;
		if (TankTickLogAccum >= 1.0f)
		{
			TankTickLogAccum = 0.f;
			UE_LOG(LogTemp, Log, TEXT("[TankAI] TickAlive Owner=%s Role=Tank BattleActive=%s"), *GetNameSafe(GetOwner()), Context->bSessionActive ? TEXT("true") : TEXT("false"));
		}
		TryRecoverAggro(DeltaTime);
	}
	if (MoveCallsAccum > 0.f)
	{
		MoveCallsAccum += DeltaTime;
		if (MoveCallsAccum >= 1.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[PartyAI] MoveCallsPerSec Owner=%s Calls=%.0f"), *GetNameSafe(GetOwner()), MoveCallsThisSecond);
			MoveCallsThisSecond = 0.f;
			MoveCallsAccum = 0.f;
		}
	}

	// Chain 시퀀스 중 이라면 작동안함
	if (Context->bInChainSequence)
	{
		State = EPartyAIState::SuppressedByChain;
		return;
	}

	const float Interval = (PresetAsset ? PresetAsset->DecisionIntervalSec : 0.25f);
	DecisionAccum += DeltaTime;
	if (DecisionAccum < Interval)
	{
		TickMovementAndAction(DeltaTime);
		LogMoveDebug(DeltaTime);
		return;
	}
	DecisionAccum = 0.f;

	// 타겟 갱신
	RefreshTarget();

	// FSM 업데이트
	UpdateStateMachine();

	// 이동 + 행동
	TickMovementAndAction(DeltaTime);
	LogMoveDebug(DeltaTime);

	// 결정 주기마다 스킬 판단
	const FJRPGCombatAIAction Best = ChooseBestAction();
	ExecuteAction(Best);
}

void UCombatPartyAIComponent::RefreshContext()
{
	Context->Role = Role;
	Context->PresetAsset = PresetAsset;
	Context->PrimaryTarget = CurrentTarget;
	Context->PartyMembers.Reset();
	if (UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
	{
		TArray<AActor*> Allies;
		Battle->GetAlliesFor(GetOwner(), Allies);
		for (AActor* Ally : Allies)
		{
			if (Ally)
			{
				Context->PartyMembers.Add(Ally);
			}
		}
	}
	Context->Refresh();
}

void UCombatPartyAIComponent::RefreshTarget()
{
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
		CurrentTarget = nullptr;
	}

	if (LastAttacker.IsValid() && !IsAliveTarget(LastAttacker.Get()))
	{
		LastAttacker = nullptr;
	}


	// 자기를 마지막으로 때린 적
	if (LastAttacker.IsValid() && IsAliveTarget(LastAttacker.Get()))
	{
		CurrentTarget = LastAttacker;
		return;
	}

	// 팀원을 공격하는 적 (ThreatComponent에서 가장 위협적인 적 찾기)
	UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
		if (Battle)
		{
			TArray<AActor*> Enemies;
			Battle->GetOpponentsFor(GetOwner(), Enemies);

			// 아군 중 누군가를 때리고 있는 적을 우선
			for (AActor* Enemy : Enemies)
			{

				if (!IsAliveTarget(Enemy)) continue;
				UThreatComponent* EnemyThreat = Enemy->FindComponentByClass<UThreatComponent>();
				if (!EnemyThreat) continue;

				TArray<AActor*> Allies;
				Battle->GetAlliesFor(GetOwner(), Allies);

				for (AActor* Ally : Allies)
				{
					if (EnemyThreat->GetThreat(Ally) > 0.f)
					{
						CurrentTarget = Enemy;
						return;
					}
				}
			}

			// 가장 가까운 적
			float ClosestDist = MAX_FLT;
			AActor* ClosestEnemy = nullptr;
			for (AActor* Enemy : Enemies)
			{

				if (!IsAliveTarget(Enemy)) continue;
				const float Dist = FVector::Dist2D(GetOwner()->GetActorLocation(), Enemy->GetActorLocation());
				if (Dist < ClosestDist)
				{
					ClosestDist = Dist;
					ClosestEnemy = Enemy;
				}
			}
			if (ClosestEnemy)
			{
				CurrentTarget = ClosestEnemy;
			}
		}
}

void UCombatPartyAIComponent::UpdateStateMachine()
{
	if (!Context->bSessionActive || Context->bSelfIsDead)
	{
		State = EPartyAIState::Recover;
		return;
	}

	if (!CurrentTarget.IsValid())
	{
		State = EPartyAIState::Follow;
		return;
	}

	const float Dist = GetDistanceToTarget();

	if (bIsRanged)
	{
		const float MinRangeWithTolerance = FMath::Max(0.f, PreferredMinRange - KeepDistanceTolerance);
		// 원거리: 너무 가까우면 거리 유지, 사거리 안이면 공격, 밖이면 추적
		if (PreferredMinRange > 0.f && Dist < MinRangeWithTolerance)
		{
			State = EPartyAIState::KeepDistance;
		}
	
		else if (Dist > AttackRange)
		{
			State = EPartyAIState::Chase;
		}
		else
		{
			State = EPartyAIState::Attack;
		}
	}
	else
	{
		// 근거리
		if (Dist <= AttackRange)
		{
			State = EPartyAIState::Attack;
		}
		else
		{
			State = EPartyAIState::Chase;
		}
	}
}

void UCombatPartyAIComponent::TickMovementAndAction(float DeltaTime)
{
	if (!CurrentTarget.IsValid()) return;
	if (Role == EJRPGPartyRole::Defender)
	{
		TankStageOneLogAccum += DeltaTime;
		if (TankStageOneLogAccum >= 1.0f)
		{
			TankStageOneLogAccum = 0.f;
			const float DistanceToTarget = GetDistanceToTarget();
			const float AcceptanceRadius = FMath::Max(AttackRange * 0.8f, 10.0f);
			const bool bMoveRequestActive = (State == EPartyAIState::Chase) || (State == EPartyAIState::KeepDistance);
			const bool bHasReachedAttackRange = (DistanceToTarget <= AttackRange);
			const TCHAR* ActionName = TEXT("Unknown");
			switch (State)
			{
			case EPartyAIState::Chase: ActionName = TEXT("MoveToTarget"); break;
			case EPartyAIState::Attack: ActionName = TEXT("Attack"); break;
			case EPartyAIState::KeepDistance: ActionName = TEXT("KeepDistance"); break;
			case EPartyAIState::Follow: ActionName = TEXT("Follow"); break;
			default: break;
			}
			UE_LOG(LogTemp, Log, TEXT("[TankAI][StageOne] Owner=%s Action=%s Target=%s DistanceToTarget=%.1f AttackRange=%.1f AcceptanceRadius=%.1f MoveRequestActive=%s HasReachedAttackRange=%s"),
				*GetNameSafe(GetOwner()),
				ActionName,
				*GetNameSafe(CurrentTarget.Get()),
				DistanceToTarget,
				AttackRange,
				AcceptanceRadius,
				bMoveRequestActive ? TEXT("true") : TEXT("false"),
				bHasReachedAttackRange ? TEXT("true") : TEXT("false"));
		}
	}
	StageOneLogAccum += DeltaTime;
	if (StageOneLogAccum >= 1.f)
	{
		StageOneLogAccum = 0.f;
		const TCHAR* ActionText = TEXT("Idle");
		if (State == EPartyAIState::Attack)
		{
			ActionText = TEXT("BasicAttack");
		}
		else if (State == EPartyAIState::Chase || State == EPartyAIState::KeepDistance)
		{
			ActionText = TEXT("MoveToTarget");
		}
		UE_LOG(LogTemp, Log, TEXT("[PartyAI] StageOne running Owner=%s Action=%s Target=%s"), *GetNameSafe(GetOwner()), ActionText, *GetNameSafe(CurrentTarget.Get()));
	}
	if (RangedRepositionPauseRemaining > 0.f)
	{
		RangedRepositionPauseRemaining = FMath::Max(0.f, RangedRepositionPauseRemaining - DeltaTime);
	}
	else if (State == EPartyAIState::KeepDistance)
	{
		// 측면 이동 방향을 주기적으로 반전해 자연스러운 스트래핑
		RangedRepositionDirection *= -1.f;
		RangedRepositionPauseRemaining = 2.f;
	}
	
	switch (State)
	{
	case EPartyAIState::Chase:
		FaceTarget(CurrentTarget.Get());
		MoveDirectlyToward(CurrentTarget->GetActorLocation());
		break;

	case EPartyAIState::Attack:
		FaceTarget(CurrentTarget.Get());
		// 근거리는 타겟에 붙어있기
		if (!bIsRanged)
		{
			const float Dist = GetDistanceToTarget();
			// 근거리는 공격 범위의 80퍼센트까지는 접근 유지
			if (Dist > AttackRange * 0.8f)
			{
				MoveDirectlyToward(CurrentTarget->GetActorLocation());
			}
		}
		break;

	case EPartyAIState::KeepDistance:

		// 원거리: 뒤로만 도망가지 않게 거리 + 측면 이동을 매 틱 적용해 부드러운 이동 유지
		/*if (RangedRepositionPauseRemaining <= 0.f)
		{
			MoveDirectlyAwayFrom(CurrentTarget->GetActorLocation(), 0.8f);
			MoveLaterallyAround(CurrentTarget->GetActorLocation(), RangedRepositionDirection * 0.55f);
			RangedRepositionPauseRemaining = 0.2f;
		}*/
		MoveDirectlyAwayFrom(CurrentTarget->GetActorLocation(), 0.8f);
		MoveLaterallyAround(CurrentTarget->GetActorLocation(), RangedRepositionDirection * 0.55f);
		FaceTarget(CurrentTarget.Get());
		break;

	default:
		break;
	}
}

FJRPGCombatAIAction UCombatPartyAIComponent::ChooseBestAction() const
{
	if (!Context->SkillComp.IsValid())
		return FJRPGCombatAIAction::MakeWait(0.f);

	// Attack 상태에서만 공격
	if (State != EPartyAIState::Attack)
		return FJRPGCombatAIAction::MakeWait(0.f);

	TWeakObjectPtr<AActor> Target = CurrentTarget;

	FJRPGCombatAIAction Best = FJRPGCombatAIAction::MakeBasicAttack(Target, 0.5f);

	{
		const float S = Scorer->ScoreAction(*Context, FJRPGCombatAIAction::MakeBasicAttack(Target, 0.f));
		if (S > Best.Score)
			Best = FJRPGCombatAIAction::MakeBasicAttack(Target, S);
	}

	TArray<FName> OwnedSkills;
	Context->SkillComp->GetOwnedSkillIds(OwnedSkills);

	for (const FName SkillId : OwnedSkills)
	{
		if (SkillId.IsNone()) continue;
		if (!Context->SkillComp->CanUseSkill(SkillId)) continue;

		const FJRPGCombatAIAction A = FJRPGCombatAIAction::MakeUseSkill(SkillId, Target, 0.f);
		const float S = Scorer->ScoreAction(*Context, A);
		if (S > Best.Score)
		{
			Best = A;
			Best.Score = S;
		}
	}

	return Best;
}

void UCombatPartyAIComponent::ExecuteAction(const FJRPGCombatAIAction& Action)
{
	if (Action.Type == EJRPGCombatAIActionType::Wait) return;

	if (Action.Type == EJRPGCombatAIActionType::BasicAttack)
	{
		if (CachedPresentation.IsValid() && Action.Target.IsValid())
		{
			CachedPresentation->TryPresentBasicAttack(Action.Target.Get());
		}		
		return;
	}

	if (Action.Type == EJRPGCombatAIActionType::UseSkill)
	{
		if (CachedPresentation.IsValid())
		{
			TArray<AActor*> Targets;
			if (IsValid(Context) && Context->SkillComp.IsValid())
			{
				Targets = BuildSkillTargets(Context->SkillComp->GetSkillDef(Action.SkillId));
			}
			if (Targets.Num() == 0 && Action.Target.IsValid())
			{
				Targets.Add(Action.Target.Get());
			}
			if (Targets.Num() <= 0)
			{
				if (Action.Target.IsValid())
				{
					CachedPresentation->TryPresentBasicAttack(Action.Target.Get());
				}
				return;
			}
			const FSkillCastResult SkillResult = CachedPresentation->TryPresentSkill(Action.SkillId, Targets, false);
			if (!SkillResult.bOk && Action.Target.IsValid())
			{
				CachedPresentation->TryPresentBasicAttack(Action.Target.Get());
			}
		}
		return;
	}
}


void UCombatPartyAIComponent::MoveDirectlyToward(const FVector& Destination)
{
	ACharacter* MyChar = Cast<ACharacter>(GetOwner());
	AAIController* AIController = MyChar ? Cast<AAIController>(MyChar->GetController()) : nullptr;
	if (!MyChar || !AIController)
	{
		return;
	}

	const FVector CurrentLocation = MyChar->GetActorLocation();
	FVector ProjectedGoal = Destination;
	bool bProjected = false;
	bool bOwnerOnNavMesh = false;
	bool bTargetOnNavMesh = false;
	bool bHasNavBoundsVolume = false;
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		TArray<AActor*> NavBoundsActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavMeshBoundsVolume::StaticClass(), NavBoundsActors);
		bHasNavBoundsVolume = NavBoundsActors.Num() > 0;
		
		FNavLocation NavLoc;
		bProjected = NavSys->ProjectPointToNavigation(Destination, NavLoc, FVector(200.f, 200.f, 300.f));
		if (bProjected)
		{
			ProjectedGoal = NavLoc.Location;
		}

		FNavLocation OwnerNav;
		FNavLocation TargetNav;
		bOwnerOnNavMesh = NavSys->ProjectPointToNavigation(CurrentLocation, OwnerNav, FVector(100.f, 100.f, 200.f));
		bTargetOnNavMesh = NavSys->ProjectPointToNavigation(Destination, TargetNav, FVector(100.f, 100.f, 200.f));
		if (NavFailureLogCooldownRemaining <= 0.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[PartyAI][NavCheck] Owner=%s OwnerOnNavMesh=%s TargetOnNavMesh=%s GoalProjected=%s HasNavBoundsVolume=%s ProjectedGoal=%s"),
				*GetNameSafe(GetOwner()), bOwnerOnNavMesh ? TEXT("true") : TEXT("false"), bTargetOnNavMesh ? TEXT("true") : TEXT("false"), bProjected ? TEXT("true") : TEXT("false"), bHasNavBoundsVolume ? TEXT("true") : TEXT("false"), *ProjectedGoal.ToString());
		}
	}
	
	const bool bNavMeshValidForMove = bProjected && (bOwnerOnNavMesh || bTargetOnNavMesh);
	if (!bNavMeshValidForMove)
	{
		if (NavFailureLogCooldownRemaining <= 0.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PartyAI][MoveBlockedByNavMesh] Owner=%s Reason=NoNavMesh RetryAfter=1.0 OwnerOnNavMesh=%s TargetOnNavMesh=%s GoalProjected=%s HasNavBoundsVolume=%s"),
				*GetNameSafe(GetOwner()),
				bOwnerOnNavMesh ? TEXT("true") : TEXT("false"),
				bTargetOnNavMesh ? TEXT("true") : TEXT("false"),
				bProjected ? TEXT("true") : TEXT("false"),
				bHasNavBoundsVolume ? TEXT("true") : TEXT("false"));
			NavFailureLogCooldownRemaining = 1.0f;
		}

		NavFailureRetryBlockRemaining = 1.0f;
		LastMoveRequestActive = false;
		LastMoveRequestResult = EPathFollowingRequestResult::Failed;
		if (bEnableNonNavMeshFallbackMovement)
		{
			const FVector ToGoal = (Destination - CurrentLocation).GetSafeNormal2D();
			if (!ToGoal.IsNearlyZero())
			{
				MyChar->AddMovementInput(ToGoal, 1.0f);
			}
		}
		return;
	}

	if (NavFailureRetryBlockRemaining > 0.f)
	{
		return;
	}
	
	const float AcceptanceRadius = FMath::Max(100.f, AttackRange * 0.8f);
	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(ProjectedGoal, AcceptanceRadius, true, true, false, false, nullptr, true);
	LastMoveRequestActive = (MoveResult == EPathFollowingRequestResult::RequestSuccessful || MoveResult == EPathFollowingRequestResult::AlreadyAtGoal);
	LastMoveRequestResult = MoveResult;
	MoveCallsThisSecond += 1.f;
	if (MoveCallsAccum <= 0.f) MoveCallsAccum = KINDA_SMALL_NUMBER;
	UE_LOG(LogTemp, Log, TEXT("[PartyAI][MoveRequest] Owner=%s MoveMethod=MoveToLocation Target=%s GoalLocation=%s AcceptanceRadius=%.1f Result=%d PathFollowingStatus=%s Controller=%s ProjectedToNavMesh=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(CurrentTarget.Get()),
		*ProjectedGoal.ToString(),
		AcceptanceRadius,
		(int32)MoveResult,
		*GetPathFollowingStatusString(),
		*GetNameSafe(AIController),
		bProjected ? TEXT("true") : TEXT("false"));
	if (MoveResult == EPathFollowingRequestResult::Failed && NavFailureLogCooldownRemaining <= 0.f)
	{
		const int32 MovementModeValue = MyChar->GetCharacterMovement() ? (int32)MyChar->GetCharacterMovement()->MovementMode.GetValue() : (int32)MOVE_None;
		UE_LOG(LogTemp, Warning, TEXT("[PartyAI][MoveFailed] Owner=%s Reason=MoveToLocationFailed PathFollowingStatus=%s CurrentLocation=%s GoalLocation=%s DistanceToGoal=%.1f Controller=%s MovementMode=%d Velocity=%s NavMeshValid=%s"),
			*GetNameSafe(GetOwner()), *GetPathFollowingStatusString(), *CurrentLocation.ToString(), *ProjectedGoal.ToString(), FVector::Dist2D(CurrentLocation, ProjectedGoal), *GetNameSafe(AIController),
			MovementModeValue, *MyChar->GetVelocity().ToString(), bProjected ? TEXT("true") : TEXT("false"));
		NavFailureLogCooldownRemaining = 1.0f;
	}
}

void UCombatPartyAIComponent::MoveDirectlyAwayFrom(const FVector& ThreatLocation, float Scale)
{
	ACharacter* MyChar = Cast<ACharacter>(GetOwner());
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
	if (FVector::DotProduct(LastMoveDirection, Dir) > 0.999f)
	{
		return;
	}
	LastMoveDirection = Dir;

	MyChar->AddMovementInput(Dir, Scale);
	MoveCallsThisSecond += 1.f;
	if (MoveCallsAccum <= 0.f) MoveCallsAccum = KINDA_SMALL_NUMBER;
}

void UCombatPartyAIComponent::MoveLaterallyAround(const FVector& FocusLocation, float Scale)
{
	ACharacter* MyChar = Cast<ACharacter>(GetOwner());
	if (!MyChar) return;

	FVector ToFocus = FocusLocation - MyChar->GetActorLocation();
	ToFocus.Z = 0.f;
	if (ToFocus.IsNearlyZero())
	{
		return;
	}

	ToFocus.Normalize();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToFocus).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		return;
	}

	if (FVector::DotProduct(LastMoveDirection, Right * FMath::Sign(Scale)) > 0.999f)
	{
		return;
	}
	LastMoveDirection = Right * FMath::Sign(Scale);

	MyChar->AddMovementInput(Right, Scale);
	MoveCallsThisSecond += 1.f;
	if (MoveCallsAccum <= 0.f) MoveCallsAccum = KINDA_SMALL_NUMBER;
}

void UCombatPartyAIComponent::TryRecoverAggro(float DeltaTime)
{
	TankReactionCooldownRemaining = FMath::Max(0.f, TankReactionCooldownRemaining - DeltaTime);
	TankDebugLogAccum += DeltaTime;
	TankBlockedLogAccum += DeltaTime;
	auto LogBlocked = [this](const TCHAR* Reason)
		{
			if (TankBlockedLogAccum >= 0.75f || LastRecoverAggroBlockReason != Reason)
			{
				UE_LOG(LogTemp, Log, TEXT("[TankAI] RecoverAggro blocked: %s"), Reason);
				LastRecoverAggroBlockReason = Reason;
				TankBlockedLogAccum = 0.f;
			}
		};
	if (Role != EJRPGPartyRole::Defender)
	{
		LogBlocked(TEXT("SelfRole is not Tank"));
		return;
	}
	if (!Context || !Context->bSessionActive)
	{
		return;
	}

	AEnemyAIController* ObservedEnemyController = nullptr;
	AActor* EnemyActor = nullptr;
	TArray<AActor*> Enemies;
	if (UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
	{
		Battle->GetOpponentsFor(GetOwner(), Enemies);
		for (AActor* Enemy : Enemies)
		{
			if (!IsValid(Enemy)) continue;
			if (APawn* EnemyPawn = Cast<APawn>(Enemy))
			{
				if (AEnemyAIController* EnemyController = Cast<AEnemyAIController>(EnemyPawn->GetController()))
				{
					ObservedEnemyController = EnemyController;
					EnemyActor = Enemy;
					break;
				}
			}
		}
	}
	if (!ObservedEnemyController)
	{
		LogBlocked(TEXT("No observed enemy"));
		return;
	}
	AActor* RawCurrentTarget = ObservedEnemyController->GetCurrentTargetActor();
	AActor* EffectiveTarget = ObservedEnemyController->GetEffectiveTargetActor();
	AActor* ForcedTarget = ObservedEnemyController->HasForcedTarget() ? EffectiveTarget : nullptr;
	AActor* EnemyCurrentTarget = EffectiveTarget;
	TankTargetDebugLogAccum += DeltaTime;
	const bool bTargetStateChanged =
		LastTargetDebugRawCurrent.Get() != RawCurrentTarget ||
		LastTargetDebugAggroTarget.Get() != RawCurrentTarget ||
		LastTargetDebugForcedTarget.Get() != ForcedTarget ||
		LastTargetDebugEffectiveTarget.Get() != EffectiveTarget;
	if (bTargetStateChanged || TankTargetDebugLogAccum >= 1.f)
	{
		UE_LOG(LogTemp, Log, TEXT("[TankAI] TargetDebug RawCurrent=%s AggroTarget=%s ForcedTarget=%s EffectiveTarget=%s Self=%s"),
			*GetNameSafe(RawCurrentTarget),
			*GetNameSafe(RawCurrentTarget),
			*GetNameSafe(ForcedTarget),
			*GetNameSafe(EffectiveTarget),
			*GetNameSafe(GetOwner()));
		TankTargetDebugLogAccum = 0.f;
		LastTargetDebugRawCurrent = RawCurrentTarget;
		LastTargetDebugAggroTarget = RawCurrentTarget;
		LastTargetDebugForcedTarget = ForcedTarget;
		LastTargetDebugEffectiveTarget = EffectiveTarget;
	}
	if (TankDebugLogAccum >= 0.75f)
	{
		UE_LOG(LogTemp, Log, TEXT("[TankAI] ObservedEnemy=%s EnemyCurrentTarget=%s SelfRole=%s"), *GetNameSafe(EnemyActor), *GetNameSafe(EnemyCurrentTarget), *RoleToDebugString(Role));
		TankDebugLogAccum = 0.f;
	}

	if (!EnemyCurrentTarget)
	{
		LogBlocked(TEXT("EnemyCurrentTarget is null"));
		return;
	}
	UCombatPartyAIComponent* TargetAI = EnemyCurrentTarget->FindComponentByClass<UCombatPartyAIComponent>();
	if (TankDebugLogAccum <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogTemp, Log, TEXT("[TankAI] EnemyTargetRole=%s"), TargetAI ? *RoleToDebugString(TargetAI->Role) : TEXT("NonParty"));
	}
	if (ObservedEnemyController->HasForcedTarget() && ForcedTarget == GetOwner())
	{
		if (!bTankAggroSuspendedByForcedSelf)
		{
			UE_LOG(LogTemp, Log, TEXT("[TankAI] AggroReaction suspended: ForcedTarget is self"));
			bTankAggroSuspendedByForcedSelf = true;
		}
		return;
	}
	if (bTankAggroSuspendedByForcedSelf)
	{
		UE_LOG(LogTemp, Log, TEXT("[TankAI] AggroReaction resumed"));
		bTankAggroSuspendedByForcedSelf = false;
	}
	if (EffectiveTarget == GetOwner())
	{
		return;
	}
	if (!IsAllyActor(EnemyCurrentTarget))
	{
		LogBlocked(TEXT("Target is not ally"));
		return;
	}
	if (TankReactionCooldownRemaining > 0.f)
	{
		LogBlocked(*FString::Printf(TEXT("Cooldown %.1fs remaining"), TankReactionCooldownRemaining));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[TankAI] Enter RecoverAggro"));
	if (TryTempTaunt(ObservedEnemyController))
	{
		TankReactionCooldownRemaining = TempTauntForcedTargetDuration + TempTauntRecoveryGracePeriod;
	}
}

bool UCombatPartyAIComponent::TryTempTaunt(AEnemyAIController* EnemyController)
{
	if (!EnemyController || !GetOwner()) return false;
	AActor* Prev = EnemyController->GetCurrentTargetActor();
	UE_LOG(LogTemp, Log, TEXT("[TankAI] Use TempTaunt"));
	EnemyController->ApplyForcedTarget(GetOwner(), TempTauntForcedTargetDuration);
	UE_LOG(LogTemp, Log, TEXT("[TankAI] EnemyTarget changed: %s -> %s"), *GetNameSafe(Prev), *GetNameSafe(GetOwner()));
	return true;
}

FString UCombatPartyAIComponent::RoleToDebugString(EJRPGPartyRole InRole) const
{
	switch (InRole)
	{
	case EJRPGPartyRole::Attacker: return TEXT("Dealer");
	case EJRPGPartyRole::Defender: return TEXT("Tank");
	case EJRPGPartyRole::Supporter: return TEXT("Supporter");
	case EJRPGPartyRole::Healer: return TEXT("Healer");
	default: return TEXT("Unknown");
	}
}

bool UCombatPartyAIComponent::IsAllyActor(AActor* Candidate) const
{
	if (!IsValid(Candidate) || !GetOwner()) return false;
	if (UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
	{
		TArray<AActor*> Allies;
		Battle->GetAlliesFor(GetOwner(), Allies);
		return Allies.Contains(Candidate);
	}
	return false;
}

void UCombatPartyAIComponent::FaceTarget(AActor* Target)
{
	if (!Target || !GetOwner()) return;

	FVector Dir = Target->GetActorLocation() - GetOwner()->GetActorLocation();
	Dir.Z = 0.f;
	if (!Dir.IsNearlyZero())
	{
		GetOwner()->SetActorRotation(Dir.Rotation());
	}
}

float UCombatPartyAIComponent::GetDistanceToTarget() const
{
	if (!CurrentTarget.IsValid() || !GetOwner()) return MAX_FLT;
	return FVector::Dist2D(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
}

bool UCombatPartyAIComponent::IsPlayerControlledNow() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	return Controller && Controller->IsPlayerController();
}

FString UCombatPartyAIComponent::GetPathFollowingStatusString() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AAIController* AIController = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	const UPathFollowingComponent* PathComp = AIController ? AIController->GetPathFollowingComponent() : nullptr;
	return PathComp ? UEnum::GetValueAsString(PathComp->GetStatus()) : TEXT("NoPathFollowing");
}

void UCombatPartyAIComponent::LogMoveDebug(float DeltaTime)
{
	MoveDebugAccum += DeltaTime;
	if (MoveDebugAccum < 1.0f)
	{
		return;
	}
	MoveDebugAccum = 0.f;

	ACharacter* MyChar = Cast<ACharacter>(GetOwner());
	if (!MyChar) return;

	const FVector CurrentLocation = MyChar->GetActorLocation();
	const FVector PreviousDebugLocation = LastDebugLocation;
	const float LocationDelta = bHasLastDebugLocation ? FVector::Dist2D(CurrentLocation, PreviousDebugLocation) : 0.f;
	LastDebugLocation = CurrentLocation;
	bHasLastDebugLocation = true;

	const float DistanceToTarget = GetDistanceToTarget();
	const float DistanceDelta = bHasLastDistanceToTarget ? (LastDistanceToTarget - DistanceToTarget) : 0.f;
	LastDistanceToTarget = DistanceToTarget;
	bHasLastDistanceToTarget = true;

	const FVector Velocity = MyChar->GetVelocity();
	const float Speed = Velocity.Size2D();
	const UCharacterMovementComponent* MoveComp = MyChar->GetCharacterMovement();
	const bool bCanMove = MoveComp && MoveComp->MovementMode != MOVE_None && MoveComp->UpdatedComponent != nullptr;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	UE_LOG(LogTemp, Log, TEXT("[PartyAI][MoveDebug] Owner=%s Target=%s CurrentLocation=%s LastLocation=%s LocationDelta=%.1f DistanceToTarget=%.1f DistanceDelta=%.1f Velocity=%s Speed=%.1f MovementMode=%d MaxWalkSpeed=%.1f ControllerName=%s IsPlayerControlled=%s IsAIControlled=%s PathFollowingStatus=%s MoveRequestActive=%s LastMoveRequestResult=%d bCanMove=%s bIsActionLocked=%s bIsAttacking=%s bIsCastingSkill=%s bUseControllerDesiredRotation=%s bOrientRotationToMovement=%s"),
		*GetNameSafe(GetOwner()), *GetNameSafe(CurrentTarget.Get()), *CurrentLocation.ToString(), *PreviousDebugLocation.ToString(), LocationDelta, DistanceToTarget, DistanceDelta, *Velocity.ToString(), Speed,
		MoveComp ? (int32)MoveComp->MovementMode : -1, MoveComp ? MoveComp->MaxWalkSpeed : 0.f, *GetNameSafe(Controller),
		IsPlayerControlledNow() ? TEXT("true") : TEXT("false"), Cast<AAIController>(Controller) ? TEXT("true") : TEXT("false"),
		*GetPathFollowingStatusString(), LastMoveRequestActive ? TEXT("true") : TEXT("false"), (int32)LastMoveRequestResult,
		bCanMove ? TEXT("true") : TEXT("false"),
		Context && Context->bInChainSequence ? TEXT("true") : TEXT("false"),
		TEXT("false"),
		CachedPresentation.IsValid() && CachedPresentation->HasActivePresentation() ? TEXT("true") : TEXT("false"),
		MoveComp && MoveComp->bUseControllerDesiredRotation ? TEXT("true") : TEXT("false"),
		MoveComp && MoveComp->bOrientRotationToMovement ? TEXT("true") : TEXT("false"));
}

TArray<AActor*> UCombatPartyAIComponent::BuildSkillTargets(const USkillDataAsset* SkillDef) const
{
	TArray<AActor*> Targets;
	if (!SkillDef || !GetOwner())
		 {
		return Targets;
		}
	
		UBattleSessionSubsystem * Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
	if (!Battle)
		 {
		if (CurrentTarget.IsValid())
			{
			Targets.Add(CurrentTarget.Get());
			}
		 return Targets;
		}

		switch (SkillDef->TargetType)
		{
		case ESkillTargetType::Self:
			Targets.Add(GetOwner());
			break;
		case ESkillTargetType::AllySingle:
			if (AActor* CriticalAlly = IsValid(Context) ? Context->AllyCriticalTarget.Get() : nullptr)
				 {
				Targets.Add(CriticalAlly);
				}
			 else if (AActor* LowestHpAlly = FindLowestHpAlly())
				 {
				Targets.Add(LowestHpAlly);
				}
			 else
				 {
				Targets.Add(GetOwner());
				}
			 break;
		case ESkillTargetType::EnemySingle:
			if (CurrentTarget.IsValid())
				 {
				Targets.Add(CurrentTarget.Get());
				}
			 break;
		case ESkillTargetType::AllyAll:
			Battle->GetAlliesFor(GetOwner(), Targets);
			break;
		case ESkillTargetType::EnemyAll:
			Battle->GetOpponentsFor(GetOwner(), Targets);
			break;
		default:
			break;
			}
	
		Targets.RemoveAll([](AActor* T) { return !IsValid(T); });
	return Targets;
	}

AActor* UCombatPartyAIComponent::FindLowestHpAlly() const
{
	UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
	if (!Battle || !GetOwner())
	{
		return nullptr;
	}

	TArray<AActor*> Allies;
	Battle->GetAlliesFor(GetOwner(), Allies);

	AActor* Lowest = nullptr;
	float LowestRatio = 2.f;
	for (AActor* Ally : Allies)
	{
		if (!IsValid(Ally))
		{
			continue;
		}

		const UHPComponent* HP = Ally->FindComponentByClass<UHPComponent>();
		if (!HP || HP->IsDead())
		{
			continue;
		}

		const float Ratio = HP->GetHpRatio01();
		if (Ratio < LowestRatio)
		{
			LowestRatio = Ratio;
			Lowest = Ally;
		}
	}

	return Lowest;
}

bool UCombatPartyAIComponent::ResolveSkillMeta(USkillComponent* SkillComp, FName SkillId, FSkillAIMeta& OutMeta) const
{
	OutMeta = FSkillAIMeta();

	if (!SkillComp || SkillId.IsNone()) return false;

	if (const USkillDataAsset* Def = SkillComp->GetSkillDef(SkillId))
	{
		
		auto HasAITag = [Def](const TCHAR* TagName) -> bool
		{
			return Def->AITags.HasTagExact(FGameplayTag::RequestGameplayTag(FName(TagName), false));
		};
		
		OutMeta.bIsHeal = Def->HealPower > 0.f || HasAITag(TEXT("Heal"));
		OutMeta.bIsBreak = Def->GroggyPower > 0.f || HasAITag(TEXT("Break"));
		OutMeta.bIsDebuff = Def->ApplyStatus != nullptr || HasAITag(TEXT("Debuff"));
		OutMeta.bIsHighDps = HasAITag(TEXT("Damage")) || (Def->BasePower > 0.f && Def->AttackScale >= 1.0f);
		OutMeta.bIsCleanse = Def->DispelAnyTags.Num() > 0;
		OutMeta.bIsTaunt = HasAITag(TEXT("Taunt")) || Def->ThreatBase > 0.f || Def->ThreatFromDamageMul > 1.0f;
		OutMeta.bIsBuff = Def->ApplyStatus != nullptr && (Def->TargetType == ESkillTargetType::Self || Def->TargetType == ESkillTargetType::AllySingle || Def->TargetType == ESkillTargetType::AllyAll);
		return true;
	}

	return false;
}
