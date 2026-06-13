#include "Combat/Exploration/RewardServiceSubsystem.h"

#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/EconomySubsystem.h"

#include "Combat/Exploration/ExplorationSaveGameSubsystem.h"
#include "Combat/Exploration/ExplorationProgressSubsystem.h"

#include "Engine/World.h"
#include "UObject/UObjectIterator.h"

UInventorySubsystem* URewardServiceSubsystem::GetInventory() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>()
		       : nullptr;
}

UEconomySubsystem* URewardServiceSubsystem::GetEconomy() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UEconomySubsystem>()
		       : nullptr;
}

UExplorationSaveGameSubsystem* URewardServiceSubsystem::GetExploreSave() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UExplorationSaveGameSubsystem>()
		       : nullptr;
}

UExplorationProgressSubsystem* URewardServiceSubsystem::GetExploreProgress() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UExplorationProgressSubsystem>()
		       : nullptr;
}

bool URewardServiceSubsystem::RollChance(float Chance01) const
{
	return FMath::FRand() <= FMath::Clamp(Chance01, 0.f, 1.f);
}

uint64 URewardServiceSubsystem::MakeUniqueKey(const FRewardGrantRequest& Req, const FRewardEntry& E) const
{
	const uint64 H1 = HashCombineFast(GetTypeHash(Req.SourceObjectId), GetTypeHash(Req.SourceDiscoveryId));
	const uint64 H2 = HashCombineFast(GetTypeHash((uint8)E.RewardType), GetTypeHash(E.Id));
	return HashCombineFast(H1, H2);
}

FGuid URewardServiceSubsystem::MakeExpContext(const FRewardGrantRequest& Req, const FRewardEntry& E) const
{
	const uint32 A = (uint32)HashCombineFast(GetTypeHash(Req.SourceObjectId), GetTypeHash(Req.SourceDiscoveryId));
	const uint32 B = (uint32)HashCombineFast(GetTypeHash((uint8)E.RewardType), GetTypeHash(E.Id));
	const uint32 C = (uint32)HashCombineFast(GetTypeHash(E.Amount), GetTypeHash(Req.TriggerType));
	const uint32 D = 0xE0E0E0E0u;
	return FGuid(A, B, C, D);
}

ICombatExpMutator* URewardServiceSubsystem::FindExpMutator(FName& OutReason) const
{
	OutReason = NAME_None;
	if (!GetWorld())
	{
		OutReason = "Reject.NoWorld";
		return nullptr;
	}

	for (TObjectIterator<UObject> It; It; ++It)
	{
		UObject* Obj = *It;
		if (!Obj || Obj->GetWorld() != GetWorld()) continue;

		if (Obj->GetClass()->ImplementsInterface(UCombatExpMutator::StaticClass()))
		{
			if (ICombatExpMutator* M = Cast<ICombatExpMutator>(Obj))
				return M;
		}
	}

	OutReason = "Reject.ExpMutatorMissing";
	return nullptr;
}

FGrantedReward URewardServiceSubsystem::ApplyOne(const FRewardGrantRequest& Req, const FRewardEntry& E)
{
	FGrantedReward G;
	G.RewardType = E.RewardType;
	G.Id = E.Id;
	G.Amount = E.Amount;

	if (!RollChance(E.Chance))
	{
		G.bGranted = false;
		G.ReasonTag = "Skip.ChanceFailed";
		return G;
	}

	// Unique 처리
	if (E.bUnique)
	{
		if (UExplorationSaveGameSubsystem* SaveSys = GetExploreSave())
		{
			if (UExplorationSaveGame* S = SaveSys->GetSave())
			{
				const uint64 Key = MakeUniqueKey(Req, E);
				if (S->UniqueRewardClaims.Contains(Key))
				{
					G.bGranted = false;
					G.ReasonTag = "Skip.UniqueAlreadyClaimed";
					return G;
				}
				S->UniqueRewardClaims.Add(Key);
				SaveSys->MarkDirty();
			}
		}
	}

	switch (E.RewardType)
	{
	case EExplorationRewardType::Gold:
		{
			if (UEconomySubsystem* Eco = GetEconomy())
			{
				Eco->AddGold(E.Amount, Req.SourceTag);
				G.bGranted = true;
				G.ReasonTag = "Granted";
			}
			else
			{
				G.bGranted = false;
				G.ReasonTag = "Reject.MissingEconomy";
			}
			return G;
		}

	// Item 계열 -> InventorySubsystem
	case EExplorationRewardType::Material:
	case EExplorationRewardType::Consumable:
	case EExplorationRewardType::Equipment:
	case EExplorationRewardType::KeyItem:
		{
			if (UInventorySubsystem* Inv = GetInventory())
			{
				const FItemOp R = Inv->AddItem(E.Id, E.Amount, Req.SourceTag, nullptr);
				G.bGranted = R.bOk;
				G.ReasonTag = R.bOk ? "Granted" : R.ReasonTag;
			}
			else
			{
				G.bGranted = false;
				G.ReasonTag = "Reject.MissingInventory";
			}
			return G;
		}

	// Unlock/Collect/Flag -> ProgressSubsystem
	case EExplorationRewardType::MapReveal:
	case EExplorationRewardType::FastTravelNode:
	case EExplorationRewardType::TraversalUnlock:
	case EExplorationRewardType::Collectible:
	case EExplorationRewardType::Lore:
	case EExplorationRewardType::Bestiary:
	case EExplorationRewardType::PuzzleKey:
	case EExplorationRewardType::Flag:
		{
			if (UExplorationProgressSubsystem* P = GetExploreProgress())
			{
				switch (E.RewardType)
				{
				case EExplorationRewardType::MapReveal: P->UnlockMap(E.Id);
					break;
				case EExplorationRewardType::FastTravelNode: P->UnlockFastTravel(E.Id);
					break;
				case EExplorationRewardType::TraversalUnlock: P->UnlockTraversal(E.Id);
					break;
				case EExplorationRewardType::Collectible: P->AddCollectible(E.Id);
					break;
				case EExplorationRewardType::Lore: P->AddLore(E.Id);
					break;
				case EExplorationRewardType::Bestiary: P->AddBestiary(E.Id);
					break;
				case EExplorationRewardType::PuzzleKey: P->SetFlag(E.Id, true);
					break;
				case EExplorationRewardType::Flag: P->SetFlag(E.Id, true);
					break;
				default: break;
				}

				if (UExplorationSaveGameSubsystem* SaveSys = GetExploreSave())
					P->FlushToSave(SaveSys);

				G.bGranted = true;
				G.ReasonTag = "Granted";
			}
			else
			{
				G.bGranted = false;
				G.ReasonTag = "Reject.MissingProgress";
			}
			return G;
		}

	// EXP: 레벨업 시스템이 있으면 소비, 없으면 "전달만" 성공 처리
	case EExplorationRewardType::ExploreExp:
		{
			FName Reason;
			if (ICombatExpMutator* Exp = FindExpMutator(Reason))
			{
				FName OutReason;
				const bool bOk = Exp->GrantExploreExp(E.Amount, MakeExpContext(Req, E), Req.SourceTag, OutReason);
				G.bGranted = bOk;
				G.ReasonTag = bOk ? "Granted" : OutReason;
			}
			else
			{
				G.bGranted = true;
				G.ReasonTag = "Forward.ToLevelSystem";
			}
			return G;
		}

	default:
		G.bGranted = false;
		G.ReasonTag = "Reject.UnsupportedRewardType";
		return G;
	}
}

FExplorationOp URewardServiceSubsystem::GrantRewards(const FRewardGrantRequest& Req, TArray<FGrantedReward>& OutGranted)
{
	OutGranted.Reset();

	TArray<FRewardEntry> All;
	// 빌드 오류 발생
	//if (Req.Table) All.Append(Req.Table->Entries);
	All.Append(Req.DirectEntries);

	if (All.Num() == 0)
		return FExplorationOp::Fail("Reject.EmptyReward");

	for (const FRewardEntry& E : All)
		OutGranted.Add(ApplyOne(Req, E));

	return FExplorationOp::Ok();
}
