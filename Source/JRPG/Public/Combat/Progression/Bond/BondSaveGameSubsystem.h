// Source/JRPGCombat/Public/Combat/Progression/Bond/BondSaveGameSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Progression/Bond/BondSaveGame.h"
#include "BondSaveGameSubsystem.generated.h"

UCLASS()
class JRPG_API UBondSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString SlotName = TEXT("JRPG_Bond");
	
	UPROPERTY(EditAnywhere)
	int32 UserIndex = 0;
	
	UPROPERTY(EditAnywhere)
	bool bAutoSaveOnChange = true;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void LoadOrCreate();
	void SaveNow();

	UBondSaveGame* GetSave() const { return SaveObj; }
	void MarkDirty();

private:
	UPROPERTY()
	TObjectPtr<UBondSaveGame> SaveObj = nullptr;
	
	bool bDirty = false;
};
