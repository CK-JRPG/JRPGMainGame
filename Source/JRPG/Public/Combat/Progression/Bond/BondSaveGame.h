// Source/JRPGCombat/Public/Combat/Progression/Bond/BondSaveGame.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Combat/Progression/Bond/BondTypes.h"
#include "BondSaveGame.generated.h"

UCLASS()
class JRPG_API UBondSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FBondPairId, FBondState> PairStates;
	UPROPERTY()
	TMap<FBondTrioId, FBondState> TrioStates;

	UPROPERTY()
	TSet<FName> UnlockedDialogueNodes;
	UPROPERTY()
	TSet<FName> CompletedDialogueNodes;

	// 현재 파티(3인 고정) :contentReference[oaicite:24]{index=24}
	UPROPERTY()
	TArray<FName> CurrentPartyIds;

	// “유의미 진행 없으면 획득 속도 감소” 용
	UPROPERTY()
	double LastSignificantProgressReal = 0.0;

	// 캐시
	UPROPERTY()
	int32 CachedTrioLevelForParty = 1;
	UPROPERTY()
	float CachedExpBonusMultiplier = 1.0f;
};
