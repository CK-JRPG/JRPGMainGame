// Source/JRPGCombat/Public/Combat/Progression/Leveling/LevelingSaveGameSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Progression/Leveling/LevelingSaveGame.h"
#include "LevelingSaveGameSubsystem.generated.h"

UCLASS()
class JRPG_API ULevelingSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString SlotName = TEXT("JRPG_Leveling");
	UPROPERTY(EditAnywhere)
	int32 UserIndex = 0;
	UPROPERTY(EditAnywhere)
	bool bAutoSaveOnChange = true;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void LoadOrCreate();
	void SaveNow();

	ULevelingSaveGame* GetSave() const { return SaveObj; }
	void MarkDirty();

private:
	UPROPERTY()
	TObjectPtr<ULevelingSaveGame> SaveObj = nullptr;
	bool bDirty = false;
};
