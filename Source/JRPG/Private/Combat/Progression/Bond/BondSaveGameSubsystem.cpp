// Source/JRPGCombat/Private/Combat/Progression/Bond/BondSaveGameSubsystem.cpp
#include "Combat/Progression/Bond/BondSaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UBondSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreate();
}

void UBondSaveGameSubsystem::Deinitialize()
{
	if (bDirty) SaveNow();
	Super::Deinitialize();
}

void UBondSaveGameSubsystem::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
		SaveObj = Cast<UBondSaveGame>(Loaded);
	}
	if (!SaveObj)
	{
		SaveObj = Cast<UBondSaveGame>(UGameplayStatics::CreateSaveGameObject(UBondSaveGame::StaticClass()));
	}
	bDirty = false;
}

void UBondSaveGameSubsystem::SaveNow()
{
	if (!SaveObj) return;
	UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
	bDirty = false;
}

void UBondSaveGameSubsystem::MarkDirty()
{
	bDirty = true;
	if (bAutoSaveOnChange) SaveNow();
}
