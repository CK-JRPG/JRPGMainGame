#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatTimeSubsystem.generated.h"

UCLASS()
class JRPG_API UCombatTimeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UCombatTimeSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return true; }

	UFUNCTION() void EnterTactical(float DurationRealSec = 5.f, float Dilation = 0.15f);
	UFUNCTION() void ExitTactical();

	UFUNCTION() void EnterChainStop(float Dilation = 0.01f);
	UFUNCTION() void ExitChainStop();

	UFUNCTION() bool IsTacticalActive() const { return bTacticalActive; }
	UFUNCTION() bool IsChainActive() const { return bChainActive; }

private:
	bool bTacticalActive = false;
	bool bChainActive = false;

	float TacticalDilation = 0.15f;
	float ChainDilation = 0.01f;

	double TacticalEndRealTime = 0.0;

	void ApplyDilation(float Value);
};
