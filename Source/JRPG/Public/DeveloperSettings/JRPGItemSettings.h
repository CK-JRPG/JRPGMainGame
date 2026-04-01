// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Combat/Items/ItemDatabaseAsset.h"
#include "JRPGItemSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "JRPG Item Settings"))
class JRPG_API UJRPGItemSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, Category = "Database")
	TSoftObjectPtr<UItemDatabaseAsset> MainItemDB;

	static UItemDatabaseAsset* GetItemDB()
	{
		const UJRPGItemSettings* Settings = GetDefault<UJRPGItemSettings>();
		return Settings ? Settings->MainItemDB.LoadSynchronous() : nullptr;
	}
};
