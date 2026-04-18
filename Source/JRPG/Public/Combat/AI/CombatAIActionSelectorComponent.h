#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatAIActionSelectorComponent.generated.h"

class UBattleSessionSubsystem;
class UCombatTargetingSubsystem;
class USkillComponent;
class USkillDataAsset;
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

	TWeakObjectPtr<USkillComponent> SkillComp;
	TWeakObjectPtr<UCombatPresentationComponent> PresentationComp;

	UBattleSessionSubsystem* GetBattle() const;
	UCombatTargetingSubsystem* GetTargeting() const;

	float GetHPRatio(AActor* Actor) const;
	bool CanAffordSkill(const USkillDataAsset &Skill) const;

	USkillDataAsset* PickBestHealSkill(TArray<AActor*> &OutTargets) const;
	USkillDataAsset* PickBestOffensiveSkill(TArray<AActor*> &OutTargets) const;
	USkillDataAsset* PickBestAggroSkill(TArray<AActor*> &OutTargets) const;

	void ThinkAndAct();
};