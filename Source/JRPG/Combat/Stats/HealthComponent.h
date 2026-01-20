#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedNative, float /*NewHP*/, float /*MaxHP*/);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JRPG_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UPROPERTY(EditAnywhere, Category="Health") float MaxHP = 100.f;
	UPROPERTY(VisibleAnywhere, Category="Health") float CurrentHP = 100.f;

	FOnHealthChangedNative OnHealthChanged;

	float GetHPRatio() const { return (MaxHP <= 0.f) ? 0.f : (CurrentHP / MaxHP); }
	bool IsDead() const { return CurrentHP <= 0.f; }

	void ApplyDamage(float Amount);
	void ApplyHeal(float Amount);

protected:
	virtual void BeginPlay() override;

private:
	void Broadcast();
};
