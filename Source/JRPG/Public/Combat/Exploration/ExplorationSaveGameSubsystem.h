#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Exploration/ExplorationSaveGame.h"
#include "ExplorationSaveGameSubsystem.generated.h"

UCLASS()
class JRPG_API UExplorationSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString SlotName = TEXT("JRPG_Explore");
	
	UPROPERTY(EditAnywhere)
	int32 UserIndex = 0;
	
	UPROPERTY(EditAnywhere)
	bool bAutoSaveOnChange = true;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION()
	void LoadOrCreate();
	UFUNCTION()
	void SaveNow();

	UExplorationSaveGame* GetSave() const { return SaveObj; }

	void MarkDirty();

private:
	UPROPERTY()
	TObjectPtr<UExplorationSaveGame> SaveObj = nullptr;
	bool bDirty = false;
};
