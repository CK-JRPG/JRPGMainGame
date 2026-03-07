#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAPChanged, int32 /*Old*/, int32 /*New*/, FName /*Reason*/);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UAPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAPComponent();

	UPROPERTY(EditAnywhere) int32 MaxAP = 10;
	UPROPERTY(VisibleAnywhere) int32 CurrentAP = 10;

	FOnAPChanged OnAPChanged;

	void InitializeAP(int32 InMaxAP, bool bFillToMax = true);

	int32 GetMaxAP() const { return MaxAP; }
	int32 GetAP() const { return CurrentAP; }

	bool CanConsume(int32 Amount) const { return Amount >=0 && CurrentAP >= Amount; }

	bool Consume(int32Amount, FNameReasonTag);
	void Restore(int32Amount, FNameReasonTag);
};