#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Characters/PartySaveGame.h"
#include "PartySaveGameSubsystem.generated.h"

UCLASS()
class JRPG_API UPartySaveGameSubsystem :public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere) FString SlotName = TEXT("JRPG_Party");
	UPROPERTY(EditAnywhere) int32 UserIndex = 0;
	UPROPERTY(EditAnywhere) bool bAutoSaveOnChange = true;

	virtual void Initialize(FSubsystemCollectionBase &Collection) override;
	virtual void Deinitialize() override;

	void LoadOrCreate();
	void SaveNow();
	UPartySaveGame* GetSave() const { return SaveObj; }
	void MarkDirty();

private:
	UPROPERTY()TObjectPtr<UPartySaveGame> SaveObj = nullptr;
	bool bDirty = false;
};