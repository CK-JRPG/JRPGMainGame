// Source/JRPGCombat/Public/Combat/Exploration/ExplorationSaveGameSubsystem.h
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
	UPROPERTY(EditAnywhere) FString SlotName = TEXT("JRPG_Explore");
	UPROPERTY(EditAnywhere) int32 UserIndex = 0;

	// 자동 저장(졸작 규모에선 on-change 저장이 가장 안전)
	UPROPERTY(EditAnywhere) bool bAutoSaveOnChange = true;

	UFUNCTION() void LoadOrCreate();
	UFUNCTION() void SaveNow();

	UExplorationSaveGame* GetSave() const { return SaveObj; }

	// Helper: 변경 후 저장
	void MarkDirty();

private:
	UPROPERTY() TObjectPtr<UExplorationSaveGame> SaveObj = nullptr;
	bool bDirty = false;
};