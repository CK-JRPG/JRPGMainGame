#include "Combat/Exploration/ExplorationProgressSubsystem.h"

void UExplorationProgressSubsystem::InitializeFromSave(UExplorationSaveGameSubsystem* SaveSys)
{
	if (!SaveSys || !SaveSys->GetSave()) return;
	UExplorationSaveGame* S = SaveSys->GetSave();

	MapReveals = S->MapReveals;
	FastTravelNodes = S->FastTravelNodes;
	TraversalUnlocks = S->TraversalUnlocks;
	Collectibles = S->Collectibles;
	LoreEntries = S->LoreEntries;
	BestiaryEntries = S->BestiaryEntries;
	Flags = S->WorldFlags;
}

void UExplorationProgressSubsystem::FlushToSave(UExplorationSaveGameSubsystem* SaveSys) const
{
	if (!SaveSys || !SaveSys->GetSave()) return;
	UExplorationSaveGame* S = SaveSys->GetSave();

	S->MapReveals = MapReveals;
	S->FastTravelNodes = FastTravelNodes;
	S->TraversalUnlocks = TraversalUnlocks;
	S->Collectibles = Collectibles;
	S->LoreEntries = LoreEntries;
	S->BestiaryEntries = BestiaryEntries;
	S->WorldFlags = Flags;

	SaveSys->MarkDirty();
}

void UExplorationProgressSubsystem::SetFlag(FName FlagId, bool bValue)
{
	if (FlagId.IsNone()) return;
	if (bValue) Flags.Add(FlagId);
	else Flags.Remove(FlagId);
}

void UExplorationProgressSubsystem::UnlockMap(FName AreaId)
{
	if (!AreaId.IsNone()) MapReveals.Add(AreaId);
}

void UExplorationProgressSubsystem::UnlockFastTravel(FName NodeId)
{
	if (!NodeId.IsNone()) FastTravelNodes.Add(NodeId);
}

void UExplorationProgressSubsystem::UnlockTraversal(FName UnlockId)
{
	if (!UnlockId.IsNone()) TraversalUnlocks.Add(UnlockId);
}

void UExplorationProgressSubsystem::AddCollectible(FName CollectibleId)
{
	if (!CollectibleId.IsNone()) Collectibles.Add(CollectibleId);
}

void UExplorationProgressSubsystem::AddLore(FName LoreId)
{
	if (!LoreId.IsNone()) LoreEntries.Add(LoreId);
}

void UExplorationProgressSubsystem::AddBestiary(FName MonsterId)
{
	if (!MonsterId.IsNone()) BestiaryEntries.Add(MonsterId);
}
