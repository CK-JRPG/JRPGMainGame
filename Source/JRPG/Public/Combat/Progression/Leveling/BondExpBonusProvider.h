// Source/JRPGCombat/Public/Combat/Progression/Leveling/BondExpBonusProvider.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BondExpBonusProvider.generated.h"

// 레벨업 문서: BondExpBonusMultiplier 조회/적용 (SSOT) :contentReference[oaicite:22]{index=22}
UINTERFACE(MinimalAPI)
class UBondExpBonusProvider : public UInterface
{
	GENERATED_BODY()
};

class IBondExpBonusProvider
{
	GENERATED_BODY()

public:
	virtual float GetBondExpBonusMultiplier() const = 0; // 예: 1.00 ~ 1.08
};
