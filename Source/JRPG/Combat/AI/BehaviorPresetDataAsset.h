#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JRPG/Combat/CombatTypes.h"
#include "BehaviorPresetDataAsset.generated.h"

USTRUCT()
struct FTagScore
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly) FGameplayTag Tag;
	UPROPERTY(EditDefaultsOnly) float Score = 0.f;
};

UCLASS()
class JRPG_API UBehaviorPresetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Preset") EBehaviorPresetType PresetType = EBehaviorPresetType::Basic;

	UPROPERTY(EditDefaultsOnly, Category="Threshold") float EmergencySelfHPRatio = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category="AP") int32 MinAPReserve = 0;

	UPROPERTY(EditDefaultsOnly, Category="Rules") bool bForceDpsCutWhenTargeted = false;
	UPROPERTY(EditDefaultsOnly, Category="Rules") bool bAllowDpsHighWhenTargeted = true;

	UPROPERTY(EditDefaultsOnly, Category="Scores") TArray<FTagScore> TagScores;

	float GetScore(const FGameplayTag& Tag) const;
};