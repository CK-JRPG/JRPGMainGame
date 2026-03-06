// Source/JRPGCombat/Public/Combat/Exploration/ExplorationSaveGame.h
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
	// OneTime 완료 플래그(오브젝트 단위) :contentReference[oaicite:26]{index=26}
	UPROPERTY() TSet<FGuid> CompletedOneTimeObjects;

	// Respawn: 다음 가능 시간(RealTime 기준 저장해두면 세션/시간배율 영향 최소화)
	UPROPERTY() TMap<FGuid, double> RespawnAvailableAtReal;

	// Discovery 저장(맵 표식 유지) :contentReference[oaicite:27]{index=27}
	UPROPERTY() TMap<FName, FDiscoveryRecord> DiscoveryMap;

	// Unique reward claims: (SourceKey + RewardKey) 합성
	UPROPERTY() TSet<uint64> UniqueRewardClaims;

	// Unlock/Flag 저장(맵해금/거점/퍼즐키 등)
	UPROPERTY() TSet<FName> MapReveals;
	UPROPERTY() TSet<FName> FastTravelNodes;
	UPROPERTY() TSet<FName> TraversalUnlocks;
	UPROPERTY() TSet<FName> Collectibles;
	UPROPERTY() TSet<FName> LoreEntries;
	UPROPERTY() TSet<FName> BestiaryEntries;
	UPROPERTY() TSet<FName> PuzzleFlags;// PuzzleKey/Flag

	// (확장) 상점/지역 플래그도 여기로(상점 시스템에서 사용)
	UPROPERTY() TSet<FName> WorldFlags;
};