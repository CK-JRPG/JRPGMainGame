#pragma once

#include "CoreMinimal.h"
#include "Combat/Items/ItemTypes.h"
#include "InventoryTypes.generated.h"

USTRUCT(BlueprintType)
struct FInventoryResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bOk = false;
	UPROPERTY(BlueprintReadOnly) FName ReasonTag = NAME_None;

	static FInventoryResult Ok()
	{
		FInventoryResult Out;
		Out.bOk = true;
		return Out;
	}

	static FInventoryResult Fail(FName InReason)
	{
		FInventoryResult Out;
		Out.bOk = false;
		Out.ReasonTag = InReason;
		return Out;
	}
};

USTRUCT(BlueprintType)
struct FFinalStatsSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float Attack = 0.f;
	UPROPERTY(BlueprintReadOnly) float Defense = 0.f;
	UPROPERTY(BlueprintReadOnly) float MaxHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float BreakPower = 0.f;
	UPROPERTY(BlueprintReadOnly) float HealingPower = 0.f;
	UPROPERTY(BlueprintReadOnly) float ThreatMod = 0.f;
};

USTRUCT(BlueprintType)
struct FStatsBreakdownSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FFinalStatsSnapshot Base;
	UPROPERTY(BlueprintReadOnly) FFinalStatsSnapshot LevelUp;
	UPROPERTY(BlueprintReadOnly) FFinalStatsSnapshot Passive;
	UPROPERTY(BlueprintReadOnly) FFinalStatsSnapshot Equipment;
	UPROPERTY(BlueprintReadOnly) FFinalStatsSnapshot Temporary;
	UPROPERTY(BlueprintReadOnly) FFinalStatsSnapshot Final;
};

USTRUCT(BlueprintType)
struct FStatsPreviewDelta
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FFinalStatsSnapshot Delta;
};

USTRUCT(BlueprintType)
struct FEquipmentLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid WeaponInstanceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FGuid> AugmentSlots;

	FEquipmentLoadout()
	{
		AugmentSlots.SetNum(3);
	}
};
