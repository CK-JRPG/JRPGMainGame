#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Exploration/ExplorationRewardTypes.h"
#include "ExplorationRewardTableAsset.generated.h"

UCLASS()
class JRPG_API UExplorationRewardTableAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FName RewardTableId = NAME_None;
	UPROPERTY(EditAnywhere)
	TArray<FRewardEntry> Entries;
};
