// Source/JRPGCombat/Public/Combat/Progression/Leveling/LevelingSaveGame.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LevelingSaveGame.generated.h"

// 9.4 저장 데이터(SSOT) :contentReference[oaicite:20]{index=20}
USTRUCT()
struct FTravelExpAccumulator
{
	GENERATED_BODY()

	UPROPERTY()
	float AccumTimeSec = 0.f;
	UPROPERTY()
	float AccumDistanceCm = 0.f;

	// anti-exploit용: 마지막 샘플/마지막 지급 위치
	UPROPERTY()
	FVector LastSampleLocation = FVector::ZeroVector;
	UPROPERTY()
	FVector LastGrantLocation = FVector::ZeroVector;
};

UCLASS()
class JRPG_API ULevelingSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 PartyLevel = 1;
	UPROPERTY()
	int32 CurrentExp = 0;

	UPROPERTY()
	TSet<FName> DiscoveredAreas;
	UPROPERTY()
	TSet<FName> DiscoveredRestPoints;
	UPROPERTY()
	TSet<FGuid> ClaimedExploreRewards; // ExplorationObjectId 단위 1회성 :contentReference[oaicite:21]{index=21}

	UPROPERTY()
	FTravelExpAccumulator TravelAcc;
};
