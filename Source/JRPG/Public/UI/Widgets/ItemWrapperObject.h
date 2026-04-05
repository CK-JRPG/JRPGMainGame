#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Items/InventorySubsystem.h"
#include "ItemWrapperObject.generated.h"

UCLASS(BlueprintType)
class JRPG_API UItemWrapperObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FItemInstance ItemData;
};