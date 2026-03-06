// Source/JRPGCombat/Public/Combat/Exploration/ExplorationProgressSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Exploration/ExplorationSaveGameSubsystem.h"
#include "ExplorationProgressSubsystem.generated.h"

UCLASS()
class JRPG_API UExplorationProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void InitializeFromSave(UExplorationSaveGameSubsystem* SaveSys);

	// --- Unlocks/Flags ---
	bool HasFlag(FName FlagId) const { return Flags.Contains(FlagId); }
	void SetFlag(FName FlagId, bool bValue);

	void UnlockMap(FName AreaId);
	void UnlockFastTravel(FName NodeId);
	void UnlockTraversal(FName UnlockId);
	void AddCollectible(FName CollectibleId);
	void AddLore(FName LoreId);
	void AddBestiary(FName MonsterId);

	// Save sync
	void FlushToSave(UExplorationSaveGameSubsystem* SaveSys) const;

private:
	UPROPERTY()
	TSet<FName> Flags;
	UPROPERTY()
	TSet<FName> MapReveals;
	UPROPERTY()
	TSet<FName> FastTravelNodes;
	UPROPERTY()
	TSet<FName> TraversalUnlocks;
	UPROPERTY()
	TSet<FName> Collectibles;
	UPROPERTY()
	TSet<FName> LoreEntries;
	UPROPERTY()
	TSet<FName> BestiaryEntries;
	UPROPERTY()
	TSet<FName> PuzzleFlags;
};
