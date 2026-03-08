#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "Combat/Skills/JRPGSkillTypes.h"
#include "CombatActionComponent.generated.h"

class UCombatCharacterComponent;
class UJRPGSkillComponent;
class UBasicCombatSubsystem;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatActionComponent();

	// 기본 공격 오버라이드(0 이하이면 CharacterDef 값 사용)
	UPROPERTY(EditAnywhere) float BasicAttackBasePowerOverride = -1.f;
	UPROPERTY(EditAnywhere) float BasicAttackAttackScaleOverride = -1.f;
	UPROPERTY(EditAnywhere) float BasicAttackDefenseScaleOverride = -1.f;
	UPROPERTY(EditAnywhere) int32 BasicAttackAPCostOverride = -1;
	UPROPERTY(EditAnywhere) int32 BasicAttackSPGainOnHitOverride = -1;
	UPROPERTY(EditAnywhere) int32 BasicAttackSPGainOnKillOverride = -1;
	UPROPERTY(EditAnywhere) float BasicAttackGroggyPowerOverride = -1.f;
	UPROPERTY(EditAnywhere) float BasicAttackThreatMultiplierOverride = -1.f;

	FCombatActionResult TryBasicAttack(AActor* Target);
	FSkillCastResult TryCastSkill(FName SkillId,const TArray<AActor*>&Targets);

protected:
	virtual void BeginPlay()override;

private:
	TWeakObjectPtr<UCombatCharacterComponent> CharacterComp;
	TWeakObjectPtr<UJRPGSkillComponent> SkillComp;

	UBasicCombatSubsystem* GetCombatSubsystem() const;
};
