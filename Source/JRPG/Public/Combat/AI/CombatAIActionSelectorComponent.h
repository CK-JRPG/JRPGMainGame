#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatAIActionSelectorComponent.generated.h"

class UCombatBattleSessionSubsystem;
class UCombatTargetingSubsystem;
class UJRPGSkillComponent;
class UJRPGSkillDataAsset;
class UCombatPresentationComponent;

UCLASS(ClassGroup=(Combat),meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatAIActionSelectorComponent :public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatAIActionSelectorComponent();

	UPROPERTY(EditAnywhere) bool bAutoDriveEnemyAI = true;
	UPROPERTY(EditAnywhere) float ThinkIntervalSec = 0.20f;

	UPROPERTY(EditAnywhere) float HealThresholdRatio = 0.45f;
	UPROPERTY(EditAnywhere) float EmergencyHealThresholdRatio = 0.25f;

protected:
	virtual void BeginPlay()override;
	virtual void TickComponent(float DeltaTime,ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float ThinkAccumulator = 0.f;

	TWeakObjectPtr<UJRPGSkillComponent> SkillComp;
	TWeakObjectPtr<UCombatPresentationComponent> PresentationComp;

	UCombatBattleSessionSubsystem* GetBattle() const;
	UCombatTargetingSubsystem* GetTargeting() const;

	float GetHPRatio(AActor* Actor) const;
	bool CanAffordSkill(const UJRPGSkillDataAsset &Skill) const;

	UJRPGSkillDataAsset* PickBestHealSkill(TArray<AActor*> &OutTargets) const;
	UJRPGSkillDataAsset* PickBestOffensiveSkill(TArray<AActor*> &OutTargets) const;

	void ThinkAndAct();
};