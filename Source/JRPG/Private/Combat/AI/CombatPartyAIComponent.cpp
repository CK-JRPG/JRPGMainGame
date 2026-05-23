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

void UCombatPartyAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Context || !Scorer) return;

	if (Context->bSelfIsDead)
	{
		return;
	}

	bool bPlayerControlled = false;
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (AController* C = P->GetController())
		{
			if (C->IsPlayerController())
			{
				bPlayerControlled = true;
			}
		}
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
		// 결정 주기 사이에도 이동은 계속
		if (!bPlayerControlled)
		{
			TickMovementAndAction(DeltaTime);
		}
		return;
	}
	DecisionAccum = 0.f;

	// 타겟 갱신
	RefreshTarget();

	if (bPlayerControlled)
	{
		State = CurrentTarget.IsValid() ? EPartyAIState::Attack : EPartyAIState::Follow;
		if (CurrentTarget.IsValid() && GetDistanceToTarget() <= AttackRange)
		{
			ExecuteAction(FJRPGCombatAIAction::MakeBasicAttack(CurrentTarget, 0.5f));
		}
		return;
	}

	// FSM 업데이트
	UpdateStateMachine();

	// 이동 + 행동
	TickMovementAndAction(DeltaTime);

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
	if (!MyChar) return;

	FVector Dir = Destination - MyChar->GetActorLocation();
	Dir.Z = 0.f;

	const float Dist = Dir.Size();
	if (Dist < 10.0f) return;

	Dir /= Dist;
	if (FVector::DotProduct(LastMoveDirection, Dir) > 0.999f)
	{
		return;
	}
	LastMoveDirection = Dir;
	MoveCallsThisSecond += 1.f;
	if (MoveCallsAccum <= 0.f) MoveCallsAccum = KINDA_SMALL_NUMBER;
	MyChar->AddMovementInput(Dir, 1.0f);
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
	AActor* EnemyCurrentTarget = ObservedEnemyController->GetEffectiveTargetActor();
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
	if (ObservedEnemyController->HasForcedTarget())
	{
		LogBlocked(TEXT("Already forced target"));
		return;
	}

	if (EnemyCurrentTarget == GetOwner())
	{
		LogBlocked(TEXT("EnemyCurrentTarget is self"));
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
		TankReactionCooldownRemaining = TempTauntForcedTargetDuration;
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
