#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatMotionTypes.h"
#include "CombatMotionPresetDataAsset.generated.h"

UCLASS()
class JRPGCOMBAT_API UCombatMotionPresetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly) FName PresetId = NAME_None;
	UPROPERTY(EditDefaultsOnly) FCombatMotionRequest Template;

	// 방향 자동 계산 옵션(선택)
	UPROPERTY(EditDefaultsOnly) bool bUseInstigatorToTargetDirection = false;
	UPROPERTY(EditDefaultsOnly) bool bUseTargetToInstigatorDirection = false;
};
