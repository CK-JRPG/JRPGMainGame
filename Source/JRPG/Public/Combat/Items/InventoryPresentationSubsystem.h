#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/InventoryTypes.h"
#include "InventoryPresentationSubsystem.generated.h"

class UCombatStatsComponent;

UCLASS()
class JRPG_API UInventoryPresentationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	FStatsPreviewDelta PreviewAugmentDelta(UCombatStatsComponent* StatsComponent, const FAugmentModifierSet& CandidateMods) const;

	UFUNCTION(BlueprintCallable)
	TArray<FItemInstance> FilterByType(const UInventorySubsystem* Inventory, EItemType ItemType) const;

	UFUNCTION(BlueprintCallable)
	TArray<FItemInstance> SearchByName(const UInventorySubsystem* Inventory, const FString& Keyword) const;
};
