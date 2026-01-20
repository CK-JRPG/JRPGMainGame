#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAPChangedNative, int32 /*New*/, int32 /*Max*/);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JRPG_API UAPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAPComponent();

	UPROPERTY(EditAnywhere, Category="AP") int32 MaxAP = 5;
	UPROPERTY(VisibleAnywhere, Category="AP") int32 CurrentAP = 0;

	UPROPERTY(EditAnywhere, Category="AP") float RegenIntervalSec = 1.0f;

	FOnAPChangedNative OnAPChanged;

	bool CanSpend(int32 Cost) const { return CurrentAP >= Cost; }
	void Spend(int32 Cost);

	void StartRegen();
	void StopRegen();

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle RegenTimer;
	void TickRegen();
	void Broadcast();
};
