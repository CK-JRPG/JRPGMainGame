// Source/JRPGCombat/Public/Combat/AI/EnemyAIController.h
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Combat/AI/CombatAITypes.h"
#include "EnemyAIController.generated.h"

class UCombatAIPresetAsset;
class UThreatComponent;

UCLASS()
class JRPG_API AEnemyAIController :public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	// 공용 프리셋(프로젝트 단위) 또는 적 종류별 교체
	UPROPERTY(EditAnywhere,Category="AI") 
	TObjectPtr<UCombatAIPresetAsset> PresetAsset = nullptr;

protected:
	virtual void OnPossess(APawn *InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

private:
	FTimerHandle DecisionTimer;

	EEnemyCombatAIState State = EEnemyCombatAIState::Idle;

	// Engage/패턴용 런타임 상태
	TMap<FName,float> SkillNextAvailableReal; // internal cooldown
	int32 OpeningUses = 0;

	TWeakObjectPtr<UThreatComponent> CachedThreat;

	void StartLoop();
	void StopLoop();
	void ThinkOnce();

	void RefreshStateFromSystems();
	EEnemyCombatAIState ComputeState() const;

	void EnsureEngageInitialThreat();
	AActor* ResolveCurrentTarget();

	bool IsSuppressedByChain() const;
	void ApplySuppressedStop();

	float NowReal() const;
};