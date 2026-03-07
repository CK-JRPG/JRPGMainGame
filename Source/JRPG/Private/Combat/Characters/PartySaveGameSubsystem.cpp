#include "Combat/Characters/PartySaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UPartySaveGameSubsystem::Initialize(FSubsystemCollectionBase&Collection)
{
	Super::Initialize(Collection);
	
	LoadOrCreate();
}

void UPartySaveGameSubsystem::Deinitialize()
{
	if (bDirty)SaveNow(); 
	
	Super::Deinitialize();
}

void UPartySaveGameSubsystem::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		USaveGame *Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
		SaveObj = Cast<UPartySaveGame>(Loaded);
	}
	
	if (!SaveObj)SaveObj = Cast<UPartySaveGame>(UGameplayStatics::CreateSaveGameObject(UPartySaveGame::StaticClass()));
	bDirty = false;
}

void UPartySaveGameSubsystem::SaveNow()
{
	if (!SaveObj)
		return;
	
	UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
	bDirty = false;
}

void UPartySaveGameSubsystem::MarkDirty()
{
	bDirty = true;
	
	if (bAutoSaveOnChange)
		SaveNow();
}