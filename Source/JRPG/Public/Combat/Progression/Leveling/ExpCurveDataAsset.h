// Source/JRPGCombat/Public/Combat/Progression/Leveling/ExpCurveDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ExpCurveDataAsset.generated.h"

// 4.2 EXP 곡선(테이블 기반) (SSOT) :contentReference[oaicite:12]{index=12}
UCLASS()
class JRPG_API UExpCurveDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 LevelMin = 1;
	UPROPERTY(EditAnywhere)
	int32 LevelMax = 30;

	// index = Level, value = ExpToNext
	// LevelMax까지 최소 LevelMax+1 길이를 권장
	UPROPERTY(EditAnywhere)
	TArray<int32> ExpToNextTable;

	int32 GetExpToNext(int32 Level) const
	{
		if (Level < LevelMin) return 0;
		if (Level >= LevelMax) return INT32_MAX; // 만렙이면 더 이상 요구치 의미 없음
		if (ExpToNextTable.IsValidIndex(Level))
			return FMath::Max(0, ExpToNextTable[Level]);

		// 테이블 누락 시 보수적 fallback (밸런스는 데이터로)
		return 100 + (Level * 50);
	}
};
