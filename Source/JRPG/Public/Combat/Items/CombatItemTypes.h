// Source/JRPGCombat/Public/Combat/Items/CombatItemTypes.h
#pragma once

#include "CoreMinimal.h"
#include "CombatItemTypes.generated.h"

USTRUCT()
struct FCombatItemApplyBreakdown
{
	GENERATED_BODY()

	UPROPERTY()
	float TotalHealedHP = 0.f;
	UPROPERTY()
	int32 TotalRestoredAP = 0;
	UPROPERTY()
	int32 TotalGrantedSP = 0;

	UPROPERTY()
	float TotalDealtDamage = 0.f;
	UPROPERTY()
	float TotalGroggyDamage = 0.f;
	UPROPERTY()
	float TotalThreatAdded = 0.f;

	UPROPERTY()
	int32 StatusAppliedCount = 0;
	UPROPERTY()
	int32 StatusRemovedCount = 0;
};

USTRUCT()
struct FCombatItemUseRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> User;
	UPROPERTY()
	FName ItemId = NAME_None;
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Targets;

	UPROPERTY(EditAnywhere)
	bool bFromTacticalReservation = false;
	UPROPERTY(EditAnywhere)
	FName ReasonTag = "Combat.UseItem";
};

USTRUCT()
struct FCombatItemUseResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bOk = false;
	UPROPERTY()
	FName ReasonTag = NAME_None;

	UPROPERTY()
	TWeakObjectPtr<AActor> User;
	UPROPERTY()
	FName ItemId = NAME_None;
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Targets;

	UPROPERTY()
	FCombatItemApplyBreakdown Breakdown;

	static FCombatItemUseResult Ok()
	{
		FCombatItemUseResult R;
		R.bOk = true;
		return R;
	}

	static FCombatItemUseResult Fail(FName Reason)
	{
		FCombatItemUseResult R;
		R.bOk = false;
		R.ReasonTag = Reason;
		return R;
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatItemUsed, const FCombatItemUseResult&/*Result*/);
