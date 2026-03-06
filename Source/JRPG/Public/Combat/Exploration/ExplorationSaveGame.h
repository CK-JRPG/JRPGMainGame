#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Combat/Exploration/ExplorationTypes.h"
#include "ExplorationSaveGame.generated.h"

UCLASS()
class JRPG_API UExplorationSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// OneTime 완료
	UPROPERTY()
	TSet<FGuid> CompletedOneTimeObjects;

	// Respawn: 다음 활성화 가능 시각(RealTime)
	UPROPERTY()
	TMap<FGuid, double> RespawnAvailableAtReal;

	// Discovery 저장(맵 표식/도감 등)
	UPROPERTY()
	TMap<FName, FDiscoveryRecord> DiscoveryMap;

	// Unique reward claim (Source + RewardKey)
	UPROPERTY()
	TSet<uint64> UniqueRewardClaims;

	// Unlock/Collect/Flags
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
	TSet<FName> WorldFlags; // 상점 오픈/퍼즐 클리어 등
};
