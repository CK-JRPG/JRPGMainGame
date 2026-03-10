// Source/JRPGCombat/Public/Combat/Items/ItemCapSettingsDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemCapSettingsDataAsset.generated.h"

UCLASS()
class JRPG_API UItemCapSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TMap<FName, float> CapByGroupPct;

	float GetCapPct(FName CapGroup,float DefaultIfMissing = 9999.f) const
	{
		if (CapGroup.IsNone()) 
			return DefaultIfMissing;
		
		if (const float* V =CapByGroupPct.Find(CapGroup)) 
			return *V;
		
		return DefaultIfMissing;
	}
};