#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/AI/CombatAIPresetAsset.h"
#include "Combat/AI/CombatAIActionTypes.h"
#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "CombatPartyAIComponent.generated.h"

class UCombatAIContext;
class UCombatAIScorer;
class USkillComponent;
class USkillDataAsset;
class UCombatPresentationComponent;
class AEnemyAIController;

//NavMesh 미사용 및 FSM 로직으로 구현.

UCLASS(ClassGroup=(JRPGCombat), meta=(BlueprintSpawnableComponent=false))
class JRPG_API UCombatPartyAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatPartyAIComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere) EJRPGPartyRole Role = EJRPGPartyRole::Attacker;
	UPROPERTY(EditAnywhere) TObjectPtr<UCombatAIPresetAsset> PresetAsset;

	UPROPERTY(VisibleAnywhere) EPartyAIState State = EPartyAIState::Follow;

	/** 피격 시 호출 - 자기를 때린 적을 우선 타겟으로 설정 */
	void NotifyDamagedBy(AActor* Attacker);
	void SetCurrentTarget(AActor* NewTarget);

private:
	UPROPERTY() TObjectPtr<UCombatAIContext> Context;
	UPROPERTY() TObjectPtr<UCombatAIScorer> Scorer;

	TWeakObjectPtr<UCombatPresentationComponent> CachedPresentation;

	UPROPERTY() float DecisionAccum = 0.f;
	UPROPERTY() TWeakObjectPtr<AActor> CurrentTarget;
	UPROPERTY() TWeakObjectPtr<AActor> LastAttacker;  // 자기를 마지막으로 때린 적

	// 캐릭터 데이터에서 가져온 사거리 파라미터
	float AttackRange = 200.f;
	float PreferredMinRange = 0.f;
	float ChaseLeashRange = 1200.f;
	bool bIsRanged = false;

	void RefreshContext();
	void UpdateStateMachine();
	void TickMovementAndAction(float DeltaTime);
	FJRPGCombatAIAction ChooseBestAction() const;
	void ExecuteAction(const FJRPGCombatAIAction& Action);

	void RefreshTarget();
	void TryRecoverAggro(float DeltaTime);
	bool TryTempTaunt(AEnemyAIController* EnemyController);
	FString RoleToDebugString(EJRPGPartyRole InRole) const;
	bool IsAllyActor(AActor* Candidate) const;
	void MoveDirectlyToward(const FVector& Destination);
	void MoveDirectlyAwayFrom(const FVector& ThreatLocation, float Scale = 1.0f);
	void MoveLaterallyAround(const FVector& FocusLocation, float Scale = 1.0f);
	void FaceTarget(AActor* Target);
	float GetDistanceToTarget() const;
	bool IsPlayerControlledNow() const;
	void LogMoveDebug(float DeltaTime);
	FString GetPathFollowingStatusString() const;
	void LoadRangeParams();
	void HandlePresentationFinished(EPresentedCombatActionType Type, FName ActionId);
	TArray<AActor*> BuildSkillTargets(const USkillDataAsset* SkillDef) const;
	AActor * FindLowestHpAlly() const;

	bool ResolveSkillMeta(USkillComponent* SkillComp, FName SkillId, struct FSkillAIMeta& OutMeta) const;

	float RangedRepositionPauseRemaining = 0.f;
	float RangedRepositionDirection = 1.f;
	float KeepDistanceTolerance = 60.f;
	float TankReactionCooldownRemaining = 0.f;
	float TankTickLogAccum = 0.f;
	float TankDebugLogAccum = 0.f;
	float TankBlockedLogAccum = 0.f;
	float TankStageOneLogAccum = 0.f;
	FString LastRecoverAggroBlockReason;
	bool bTankAggroSuspendedByForcedSelf = false;
	float TankTargetDebugLogAccum = 0.f;
	TWeakObjectPtr<AActor> LastTargetDebugRawCurrent;
	TWeakObjectPtr<AActor> LastTargetDebugAggroTarget;
	TWeakObjectPtr<AActor> LastTargetDebugForcedTarget;
	TWeakObjectPtr<AActor> LastTargetDebugEffectiveTarget;
	float StageOneLogAccum = 0.f;
	float TempTauntForcedTargetDuration = 1.5f;
	float TempTauntRecoveryGracePeriod = 0.0f;
	float MoveCallsThisSecond = 0.f;
	float MoveCallsAccum = 0.f;
	FVector LastMoveDirection = FVector::ZeroVector;
	FVector LastDebugLocation = FVector::ZeroVector;
	float MoveDebugAccum = 0.f;
	float LastDistanceToTarget = 0.f;
	bool bHasLastDistanceToTarget = false;
	bool bHasLastDebugLocation = false;
	bool LastMoveRequestActive = false;
	EPathFollowingRequestResult::Type LastMoveRequestResult = EPathFollowingRequestResult::Failed;
	float NavFailureRetryBlockRemaining = 0.f;
	float NavFailureLogCooldownRemaining = 0.f;
	bool bEnableNonNavMeshFallbackMovement = true;
	bool bPrevPlayerControlled = false;
	float NextAutoAttackTime = 0.f;
	float RetryDelayUntilTime = 0.f;
	float AutoAttackBusyUntilTime = 0.f;
	float PlayerAutoAttackDebugLogRemaining = 0.f;
	float CannotPresentLogCooldownRemaining = 0.f;
	FName LastCannotPresentReasonTag = NAME_None;
	float LastAutoAttackSuccessTime = -1000.f;
	float AttackKeepRange = 650.f;
};
