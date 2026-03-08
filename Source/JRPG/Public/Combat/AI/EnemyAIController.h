// Source/JRPGCombat/Public/Combat/AI/EnemyAIController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Combat/AI/CombatAIActionTypes.h"
#include "EnemyAIController.generated.h"

class UCombatThreatComponent;
class USkillComponent;
class UCombatAIPresetAsset;

UCLASS()
class JRPG_API AEnemyAIController :public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void OnPossess(APawn *InPawn)override;
	virtual void Tick(float DeltaSeconds)override;

	UPROPERTY(EditAnywhere) TObjectPtr<UCombatAIPresetAsset> PresetAsset;

private:
	UPROPERTY() TObjectPtr<APawn> ControlledPawn;
	UPROPERTY() TObjectPtr<UCombatThreatComponent> ThreatComp;
	UPROPERTY() TObjectPtr<USkillComponent> SkillComp;

	UPROPERTY() EEnemyCombatState State = EEnemyCombatState::Idle;

	// Rising 동안 타겟 락을 늘리기 위한 간단한 타이머
	double TargetLockUntilReal = 0.0;

	void RefreshStateFromGroggyAndChain();
	void TickCombatNormal(float DeltaSeconds);
	void TickGroggyStunned(float DeltaSeconds);
	void TickRising(float DeltaSeconds);

	bool IsChainSequenceActive()const;
	bool ReadGroggy(EGroggyPhase&OutPhase)const;
};