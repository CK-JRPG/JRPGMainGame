#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CombatAIInterfaces.h"
#include "Combat/AI/CombatAIActionTypes.h"
#include "EnemyAIController.generated.h"

class UThreatComponent;
class USkillComponent;
class UCombatAIPresetAsset;
class UCombatCharacterComponent;
class UCombatPresentationComponent;
class UTargetGuideLineComponent;
class UBattleSessionSubsystem;

/**
 * 적 AI 컨트롤러 (Tales of Arise 스타일 FSM, NavMesh/BT 미사용)
 * 
 * - 어그로 최고 대상을 타겟으로 공격
 * - 근거리: 타겟이 공격 범위 밖이면 Chase → 범위 안이면 Attack
 * - 원거리: 타겟이 사거리 안이면 Attack, 너무 멀면 Chase, 너무 가까우면 Retreat
 */
UCLASS()
class JRPG_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	void ResetForNewBattle();
	AActor* GetCurrentTargetActor() const { return CurrentTarget.Get(); }
	AActor* GetEffectiveTargetActor() const;
	void ForceSetCurrentTarget(AActor* NewTarget);
	void ApplyForcedTarget(AActor* NewTarget, float DurationSec = 1.5f);
	bool HasForcedTarget() const;

	UPROPERTY(EditAnywhere) TObjectPtr<UCombatAIPresetAsset> PresetAsset;

private:
	UPROPERTY() TObjectPtr<APawn> ControlledPawn;
	UPROPERTY() TObjectPtr<UThreatComponent> ThreatComp;
	UPROPERTY() TObjectPtr<USkillComponent> SkillComp;
	UPROPERTY() TObjectPtr<UCombatCharacterComponent> CharComp;
	UPROPERTY() TObjectPtr<UCombatPresentationComponent> PresentationComp;
	UPROPERTY() TObjectPtr<UTargetGuideLineComponent> TargetGuideLineComp;

	UPROPERTY() EEnemyCombatState State = EEnemyCombatState::Idle;
	UPROPERTY() TWeakObjectPtr<AActor> CurrentTarget;
	UPROPERTY() TWeakObjectPtr<AActor> ForcedTarget;
	double ForcedTargetUntilReal = 0.0;
	double TargetSwitchLockedUntilReal = 0.0;

	UPROPERTY(EditAnywhere, Category = "JRPG|Combat|Aggro", meta = (ClampMin = "0.0"))
	float TargetSwitchLockSec = 3.0f;

	UPROPERTY(EditAnywhere, Category = "JRPG|Combat|Aggro", meta = (ClampMin = "1.0"))
	float TargetSwitchThreatRatio = 1.15f;

	UPROPERTY(EditAnywhere, Category = "JRPG|Combat|Aggro", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MaxFocusedTargetShare = 0.667f;

	// AI 사거리 파라미터 (캐릭터 데이터 에셋에서 로드)
	float AttackRange = 200.f;
	float PreferredMinRange = 0.f;
	float ChaseLeashRange = 1200.f;
	float AttackStartRange = 320.f;
	float AttackKeepRange = 430.f;
	float LeashRange = 1500.f;
	bool bIsRanged = false;

	double TargetLockUntilReal = 0.0;
	double NextAttackAllowedReal = 0.0;
	double WindupUntilReal = 0.0;
	double RecoveryUntilReal = 0.0;
	TWeakObjectPtr<AActor> WindupTarget;

	// FSM
	void RefreshStateFromGroggyAndChain();
	void RefreshTarget();
	void SetCurrentTarget(AActor* NewTarget, bool bApplySwitchLock = true);
	void TickChase(float DeltaSeconds);
	void TickAttack(float DeltaSeconds);
	void TickRetreat(float DeltaSeconds);
	void TickGroggyStunned(float DeltaSeconds);
	void TickRising(float DeltaSeconds);
	void TryExecuteOffensiveAction(AActor* Target);

	// NavMesh 미사용 직접 이동
	void MoveDirectlyToward(const FVector& Destination, float DeltaTime);
	void MoveDirectlyAwayFrom(const FVector& ThreatLocation, float DeltaTime);
	void FaceTarget(AActor* Target);

	// 유틸리티
	float GetDistanceToTarget() const;
	void LoadRangeParamsFromCharacterData();
	bool IsTargetOverFocusLimit(AActor* Candidate, const UBattleSessionSubsystem* Battle) const;
	AActor* SelectBestThreatTargetRespectingFocus(const TArray<AActor*>& Candidates, const UBattleSessionSubsystem* Battle) const;
	AActor* SelectClosestTargetRespectingFocus(const TArray<AActor*>& Candidates, const UBattleSessionSubsystem* Battle) const;

	bool IsChainSequenceActive() const;
	bool ReadGroggy(EJRPGGroggyPhase& OutPhase) const;
	
	mutable TWeakObjectPtr<UObject> CachedChainProviderObject;
	mutable double NextChainProviderRescanAt = 0.0;
	static constexpr double ChainProviderRescanIntervalSec = 1.0;
};
