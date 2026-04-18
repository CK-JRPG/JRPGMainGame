#include "Combat/Exploration/ExplorationSaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UExplorationSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreate();
}

void UExplorationSaveGameSubsystem::Deinitialize()
{
	if (bDirty) SaveNow();
	Super::Deinitialize();
}

void UExplorationSaveGameSubsystem::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
		SaveObj = Cast<UExplorationSaveGame>(Loaded);
	}

	if (!SaveObj)
	{
		SaveObj = Cast<UExplorationSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UExplorationSaveGame::StaticClass())
		);
	}

	bDirty = false;
}

void UExplorationSaveGameSubsystem::SaveNow()
{
	if (!SaveObj) return;
	UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
	bDirty = false;
}

void UExplorationSaveGameSubsystem::MarkDirty()
{
	bDirty = true;
	if (bAutoSaveOnChange) SaveNow();
}
