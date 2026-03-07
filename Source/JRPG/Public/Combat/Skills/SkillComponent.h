#pragma once

#include "CoreMinimal.h"
#include "SkillTypes.h"
#include "Components/ActorComponent.h"
#include "Combat/Skills/SkillDataAsset.h"
#include "Combat/Skills/SkillTypes.h"
#include "SkillComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSkillCast, FName /*SkillId*/, AActor* /*Caster*/, int32 /*TargetCount*/);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API USkillComponent :public UActorComponent
{
	GENERATED_BODY()
	
public:
	USkillComponent();


	UPROPERTY(EditAnywhere) 
	TArray<TObjectPtr<USkillDataAsset>> KnownSkills;

	FOnSkillCast OnSkillCast;

	bool HasSkill(FName SkillId) const;
	void LearnSkill(USkillDataAsset* Skill);

	float GetCooldownRemaining(FName SkillId) const;
	FSkillCastResult CastSkill(FName SkillId, const TArray<AActor*>& Targets, FName ReasonTag);


protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TMap<FName, float> Cooldowns;

	TWeakObjectPtr<class UCombatStatsComponent> Stats;
	TWeakObjectPtr<class UHPComponent> HP;
	TWeakObjectPtr<class UAPComponent> AP;
	TWeakObjectPtr<class USPComponent> SP;

	USkillDataAsset* FindSkill(FName SkillId) const;

	FSkillCastResult ValidateCast(const USkillDataAsset& Skill, const TArray<AActor*>& Targets) const;
	void ApplySkillEffects(const USkillDataAsset& Skill, const TArray<AActor*>& Targets);

	bool IsHostileTarget(AActor* Target) const;
};
