// Source/JRPGCombat/Public/Combat/Items/EconomySubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Items/ItemTypes.h"
#include "EconomySubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoldChanged, int32 /*Before*/, int32 /*After*/);

UCLASS()
class JRPG_API UEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) int32 Gold = 0;

	FOnGoldChanged OnGoldChanged;

	int32 GetGold() const { return Gold; }

	void AddGold(int32 Amount, FName SourceTag);
	FItemOp TrySpendGold(int32 Amount, FName ReasonTag);
};