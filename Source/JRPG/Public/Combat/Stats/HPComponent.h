#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HPComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnHPChanged, float, float, FName);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDeath, AActor*, FName);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPG_API UHPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHPComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere) float MaxHP = 100.f;
	UPROPERTY(VisibleAnywhere) float CurrentHP = 100.f;

	FOnHPChanged OnHPChanged;
	FOnDeath OnDeath;

	void InitializeHP(float InMaxHP, bool bFillToMax = true);
	// Snapshot/SaveGame import only: gameplay change/death delegates are intentionally not emitted.
	void ImportRuntimeState(float InMaxHP, float InCurrentHP);
	void SetMaxHP(float InMaxHP,bool bKeepRatio);
	void RestoreFull(FName ReasonTag);

	float GetMaxHP() const {return MaxHP;}
	float GetHP() const {return CurrentHP;}
	float GetHpRatio01() const { return MaxHP > 0.f ? FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f) : 0.f; }
	bool IsDead() const {return CurrentHP <= 0.f;}

	void ApplyDamage(float Amount, AActor *Instigator, FName ReasonTag);
	void Heal(float Amount, AActor *Instigator, FName ReasonTag);
};
