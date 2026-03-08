#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatHPComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnHPChanged, float, float, FName);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDeath, AActor*, FName);

UCLASS(ClassGroup=(Combat), meta = (BlueprintSpawnableComponent))
class JRPG_API UCombatHPComponent :public UActorComponent
{
	GENERATED_BODY()
	
public:
	UCombatHPComponent();

	UPROPERTY(EditAnywhere) float MaxHP = 100.f;
	UPROPERTY(VisibleAnywhere) float CurrentHP = 100.f;

	FOnHPChanged OnHPChanged;
	FOnDeath OnDeath;

	void InitializeHP(float InMaxHP, bool bFillToMax = true);
	void SetMaxHP(float InMaxHP,bool bKeepRatio);

	float GetMaxHP() const {return MaxHP;}
	float GetHP() const {return CurrentHP;}
	bool IsDead() const {return CurrentHP <= 0.f;}

	void ApplyDamage(float Amount,AActor *Instigator,FName ReasonTag);
	void Heal(float Amount, AActor *Instigator, FName ReasonTag);
};