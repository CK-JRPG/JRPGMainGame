// Source/JRPGCombat/Private/Combat/Exploration/RewardServiceSubsystem.cpp
#include "Combat/Exploration/RewardServiceSubsystem.h"

#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/EconomySubsystem.h"

#include "Combat/Exploration/ExplorationSaveGameSubsystem.h"
#include "Combat/Exploration/ExplorationProgressSubsystem.h"

#include "Engine/World.h"

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

double URewardServiceSubsystem::NowReal() const
{
	return GetWorld() ? (double)GetWorld()->GetRealTimeSeconds() : 0.0;
}

bool URewardServiceSubsystem::RollChance(float Chance01) const
{
	const float C = FMath::Clamp(Chance01, 0.f, 1.f);
	return FMath::FRand() <= C;
}

uint64 URewardServiceSubsystem::MakeUniqueKey(const FRewardGrantRequest& Req, const FRewardEntry& E) const
{
	// UniqueKey = hash(Source + RewardType + Id)
	const uint64 H1 = HashCombineFast(GetTypeHash(Req.SourceObjectId), GetTypeHash(Req.SourceDiscoveryId));
	const uint64 H2 = HashCombineFast(GetTypeHash((uint8)E.RewardType), GetTypeHash(E.Id));
	return HashCombineFast(H1, H2);
}

FGrantedReward URewardServiceSubsystem::ApplyOne(const FRewardGrantRequest& Req, const FRewardEntry& E)
{
	FGrantedReward G;
	G.RewardType = E.RewardType;
	G.Id = E.Id;
	G.Amount = E.Amount;

	// chance
	if (!RollChance(E.Chance))
	{
		G.bGranted = false;
		G.ReasonTag = "Skip.ChanceFailed";
		return G;
	}

	// unique
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

	// apply
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

	// Item 계열 -> InventorySubsystem로 통일
	case EExplorationRewardType::CraftMaterial:
	case EExplorationRewardType::Consumable:
	case EExplorationRewardType::Equipment:
	case EExplorationRewardType::SkillResource:
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

	// Unlock/Collect/Lore -> ProgressSubsystem
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

				// save sync
				if (UExplorationSaveGameSubsystem* SaveSys = GetExploreSave())
				{
					P->FlushToSave(SaveSys);
				}

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

	// EXP는 레벨업 시스템이 OnRewardsGranted를 받아 처리하는 게 SSOT에 더 맞음(여긴 “전달”만)
	case EExplorationRewardType::ExploreExp:
		G.bGranted = true;
		G.ReasonTag = "Forward.ToExpSystem";
		return G;

	default:
		G.bGranted = false;
		G.ReasonTag = "Reject.UnsupportedRewardType";
		return G;
	}
}

FExplorationOp URewardServiceSubsystem::GrantRewards(const FRewardGrantRequest& Req, TArray<FGrantedReward>& OutGranted)
{
	OutGranted.Reset();

	// 테이블 + 직접 엔트리 합치기
	TArray<FRewardEntry> All;
	if (Req.Table)
		All.Append(Req.Table->Entries);
	All.Append(Req.DirectEntries);

	if (All.Num() == 0)
		return FExplorationOp::Fail("Reject.EmptyReward");

	for (const FRewardEntry& E : All)
	{
		OutGranted.Add(ApplyOne(Req, E));
	}

	return FExplorationOp::Ok();
}
