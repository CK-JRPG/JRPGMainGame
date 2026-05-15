#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/AI/CombatAIPresetAsset.h"
#include "Combat/AI/CombatAIActionTypes.h"
#include "CombatPartyAIComponent.generated.h"

class UCombatAIContext;
class UCombatAIScorer;
class USkillComponent;
class USkillDataAsset;
class UCombatPresentationComponent;

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
	UPROPERTY(VisibleAnywhere) FString CurrentGoal;
	UPROPERTY(VisibleAnywhere) FString CurrentAction;
	UPROPERTY(VisibleAnywhere) float LastDecisionScore = 0.f;

	/** 피격 시 호출 - 자기를 때린 적을 우선 타겟으로 설정 */
	void NotifyDamagedBy(AActor* Attacker);

private:
	UPROPERTY() TObjectPtr<UCombatAIContext> Context;
	UPROPERTY() TObjectPtr<UCombatAIScorer> Scorer;

	TWeakObjectPtr<UCombatPresentationComponent> CachedPresentation;

	UPROPERTY() float DecisionAccum = 0.f;
	UPROPERTY() float CurrentActionElapsed = 0.f;
	UPROPERTY() TWeakObjectPtr<AActor> CurrentTarget;
	UPROPERTY() TWeakObjectPtr<AActor> LastAttacker;  // 자기를 마지막으로 때린 적
	UPROPERTY() TWeakObjectPtr<AActor> LastMoveTargetActor;
	UPROPERTY() FVector LastMoveDestination = FVector::ZeroVector;
	UPROPERTY() bool bHasLastMoveDestination = false;
	UPROPERTY() bool bWithinAttackRange = false;
	UPROPERTY() bool bQueuedDecisionRefresh = false;
	
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
	AActor * FindEnemyTargetingActor(AActor * DesiredTarget) const;
	AActor * FindTankAlly() const;
	bool HandleRoleBasedAggroReaction();
	void MoveTowardSafePointFromEnemy(AActor * EnemyActor, float Scale = 1.0f);
	void MoveBetweenEnemyAndAlly(AActor * EnemyActor, AActor * AllyActor);
	void SetDecisionDebug(const TCHAR * InGoal, const TCHAR * InAction, float InScore);
	void MoveDirectlyToward(const FVector& Destination);
	void MoveDirectlyAwayFrom(const FVector& ThreatLocation, float Scale = 1.0f);
	void MoveLaterallyAround(const FVector& FocusLocation, float Scale = 1.0f);
	void FaceTarget(AActor* Target);
	float GetDistanceToTarget() const;
	void LoadRangeParams();
	TArray<AActor*> BuildSkillTargets(const USkillDataAsset* SkillDef) const;
	AActor * FindLowestHpAlly() const;

	bool ResolveSkillMeta(USkillComponent* SkillComp, FName SkillId, struct FSkillAIMeta& OutMeta) const;

	float RangedRepositionPauseRemaining = 0.f;
	float RangedRepositionDirection = 1.f;
	float KeepDistanceTolerance = 60.f;
};
