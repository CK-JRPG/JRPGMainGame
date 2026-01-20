#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TacticalModeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTacticalEntered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTacticalExited);

UCLASS()
class JRPG_API UTacticalModeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable) FOnTacticalEntered OnTacticalEntered;
	UPROPERTY(BlueprintAssignable) FOnTacticalExited  OnTacticalExited;

	UFUNCTION() void EnterTactical(float DurationRealSec = 5.0f, float Dilation = 0.15f);
	UFUNCTION() void ExitTactical();

	UFUNCTION() bool ReserveSkillById(AActor* Character, FName SkillId);
	UFUNCTION() void ClearReservation(AActor* Character);

	UFUNCTION() bool IsTacticalActive() const;

private:
	class UCombatTimeSubsystem* GetTime() const;
};
