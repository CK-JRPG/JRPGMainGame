// Source/JRPGCombat/Private/Combat/Progression/Leveling/LevelingSaveGameSubsystem.cpp
#include "Combat/Progression/Leveling/LevelingSaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"

void ULevelingSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreate();
}

void ULevelingSaveGameSubsystem::Deinitialize()
{
	if (bDirty) SaveNow();
	Super::Deinitialize();
}

void ULevelingSaveGameSubsystem::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
		SaveObj = Cast<ULevelingSaveGame>(Loaded);
	}
	if (!SaveObj)
	{
		SaveObj = Cast<ULevelingSaveGame>(UGameplayStatics::CreateSaveGameObject(ULevelingSaveGame::StaticClass()));
	}
	bDirty = false;
}

void ULevelingSaveGameSubsystem::SaveNow()
{
	if (!SaveObj) return;
	UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
	bDirty = false;
}

void ULevelingSaveGameSubsystem::MarkDirty()
{
	bDirty = true;
	if (bAutoSaveOnChange) SaveNow();
}
