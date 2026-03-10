#pragma once

#include"CoreMinimal.h"
#include"Engine/DataAsset.h"
#include"Combat/Groggy/GroggyTypes.h"
#include"GroggySettingsDataAsset.generated.h"

/**
 * 문서의 "GroggySettings(적/종류별 테이블)"을 DataAsset 형태로 제공. :contentReference[oaicite:17]{index=17}
 */
UCLASS()
class JRPGCOMBAT_API UGroggySettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// EnemyTypeId -> Settings
	UPROPERTY(EditAnywhere)
	TMap<FName, FGroggySettings> SettingsByEnemyType;

	UFUNCTION()
	bool TryGetSettings(const FName EnemyTypeId, FGroggySettings& Out) const
	{
		if (const FGroggySettings* Found = SettingsByEnemyType.Find(EnemyTypeId))
		{
			Out = *Found;
			return true;
		}
		return false;
	}
};