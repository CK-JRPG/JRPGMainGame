#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPG/Combat/CombatTypes.h"
#include "CombatAIComponent.generated.h"

class UBehaviorPresetDataAsset;
class UHealthComponent;
class UAPComponent;
class USkillComponent;
class UCombatSkill;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatAIComponent();

	UPROPERTY(EditAnywhere, Category="AI") bool bEnabled = true;
	UPROPERTY(EditAnywhere, Category="AI") float DecisionIntervalSec = 0.2f;

	UPROPERTY(EditAnywhere, Category="AI") ECombatRole Role = ECombatRole::Attacker;
	UPROPERTY(EditAnywhere, Category="AI") TObjectPtr<UBehaviorPresetDataAsset> Preset;

	void StartAI();
	void StopAI();

	void SetMainTarget(AActor* Target) { MainTarget = Target; }
	void SetIsTargeted(bool bIn) { bIsTargeted = bIn; }

	bool IsTargeted() const { return bIsTargeted; }
	AActor* GetMainTarget() const { return MainTarget.Get(); }
	ECombatRole GetRole() const { return Role; }

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle DecisionTimer;
	TWeakObjectPtr<AActor> MainTarget;
	bool bIsTargeted = false;

	void Decide();

	UHealthComponent* GetHP() const;
	UAPComponent* GetAP() const;
	USkillComponent* GetSkills() const;

	bool IsEmergencySelf() const;
	float ScoreSkill(const UCombatSkill* Skill, bool bEmergencySelf) const;
};
