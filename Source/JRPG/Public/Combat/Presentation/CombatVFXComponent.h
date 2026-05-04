#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "Combat/Skills/SkillTypes.h"
#include "CombatVFXComponent.generated.h"

class UCombatCharacterComponent;
class UCombatPresentationComponent;
class USkillComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UCurveFloat;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatVFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatVFXComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<UCombatCharacterComponent> CharacterComp;
	TWeakObjectPtr<UCombatPresentationComponent> PresentationComp;
	TWeakObjectPtr<USkillComponent> SkillComp;

	void HandleBasicAttackResolved(const FCombatActionResult& Result);
	void HandleSkillTargetResolved(const FSkillTargetResolvedEvent& Event);

	UNiagaraComponent* SpawnTargetEffect(UNiagaraSystem* System, AActor* Target, float UniformScale, bool bCritical) const;
	float EvaluateEffectScale(float Magnitude, const UCurveFloat* Curve, float ReferenceDamage, float Multiplier, float MinScale, float MaxScale) const;
	FVector ResolveTargetEffectLocation(AActor* Target) const;
	FRotator ResolveTargetEffectRotation(AActor* Target, FVector& OutSurfaceNormal) const;
};
