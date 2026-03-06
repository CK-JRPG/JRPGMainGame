#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SPComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSPChanged, int32 /*Old*/, int32 /*New*/, FName /*Reason*/);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API USPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USPComponent();

	UPROPERTY(EditAnywhere) int32 MaxSP = 100;
	UPROPERTY(VisibleAnywhere) int32 CurrentSP = 0;

	FOnSPChanged OnSPChanged;

	void InitializeSP(int32 InMaxSP, int32 InStartSP = 0);

	int32 GetSP() const { return CurrentSP; }
	int32 GetMaxSP() const { return MaxSP; }

	void AddSP(int32 Amount, FName ReasonTag);
	bool ConsumeSP(int32 Amount, FName ReasonTag);
};