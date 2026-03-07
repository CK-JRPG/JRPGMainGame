// Source/JRPGCombat/Public/Combat/AI/CombatAIActionSelectorComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatAIActionSelectorComponent.generated.h" 

class UBattleSessionSubsystem;
class UCombatTargetingSubsystem;
class USkillComponent;
class USkillDataAsset;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatAIActionSelectorComponent :public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatAIActionSelectorComponent();

	UPROPERTY(EditAnywhere) bool bAutoDriveEnemyTurns = true;
	UPROPERTY(EditAnywhere) float ThinkDelaySec = 0.15f;

	UPROPERTY(EditAnywhere) float HealThresholdRatio = 0.45f;
	UPROPERTY(EditAnywhere) float EmergencyHealThresholdRatio = 0.25f;

protected:
	virtual void BeginPlay()override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle ThinkTimer;

	TWeakObjectPtr<USkillComponent> SkillComp;

	UBattleSessionSubsystem* GetBattle() const;
	UCombatTargetingSubsystem* GetTargeting() const;

	void HandleTurnStarted(AActor *Actor,int32 Round);
	void ThinkAndAct();

	float GetHPRatio(AActor *Actor) const;
	bool CanAffordSkill(const USkillDataAsset &Skill) const;

	USkillDataAsset* PickBestHealSkill(TArray<AActor*> &OutTargets) const;
	USkillDataAsset* PickBestOffensiveSkill(TArray<AActor*> &OutTargets) const;
};