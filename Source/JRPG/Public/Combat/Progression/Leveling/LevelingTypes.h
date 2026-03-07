// Source/JRPGCombat/Public/Combat/Progression/Leveling/LevelingTypes.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "LevelingTypes.generated.h"

// 9.1 공용 Enum (SSOT) :contentReference[oaicite:8]{index=8}
UENUM()
enum class EExpSource : uint8
{
	Travel,
	DiscoverArea,
	DiscoverRestPoint,
	BondLevelUp,
	CombatReward,
	ExploreReward
};

UENUM()
enum class EExpGrantResult : uint8
{
	Success,
	Rejected
};

// 9.2 EXP 지급 요청(스냅샷) (SSOT) :contentReference[oaicite:9]{index=9}
USTRUCT()
struct FExpGrantRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EExpSource Source = EExpSource::Travel;
	UPROPERTY(EditAnywhere)
	int32 BaseExp = 0;

	// ContextId : GUID or Name (SSOT) :contentReference[oaicite:10]{index=10}
	UPROPERTY(EditAnywhere)
	bool bUseGuidContext = true;
	UPROPERTY(EditAnywhere)
	FGuid ContextGuid;
	UPROPERTY(EditAnywhere)
	FName ContextName = NAME_None;

	// 텔레메트리/디버그
	UPROPERTY(EditAnywhere)
	FName SourceTag = "Exp.Granted";
	UPROPERTY(EditAnywhere)
	double TimestampReal = 0.0;

	// 확장 슬롯(전투 tier 등): 호출자가 BaseExp 산정 후 채워도 되고, 여기 Param으로 남겨도 됨
	UPROPERTY(EditAnywhere)
	int32 ParamA = 0;
	UPROPERTY(EditAnywhere)
	int32 ParamB = 0;

	bool HasValidContext() const
	{
		return bUseGuidContext ? ContextGuid.IsValid() : !ContextName.IsNone();
	}
};

USTRUCT()
struct FExpGrantSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	EExpSource Source = EExpSource::Travel;
	UPROPERTY()
	int32 BaseExp = 0;
	UPROPERTY()
	int32 FinalExp = 0;
	UPROPERTY()
	float BondMultiplierApplied = 1.0f;

	UPROPERTY()
	bool bUseGuidContext = true;
	UPROPERTY()
	FGuid ContextGuid;
	UPROPERTY()
	FName ContextName = NAME_None;

	UPROPERTY()
	double TimestampReal = 0.0;
};

USTRUCT()
struct FExpGrantOp
{
	GENERATED_BODY()

	UPROPERTY()
	bool bOk = false;
	UPROPERTY()
	EExpGrantResult Result = EExpGrantResult::Rejected;
	UPROPERTY()
	FName ReasonTag = NAME_None;
	UPROPERTY()
	FExpGrantSnapshot Snapshot;

	static FExpGrantOp Ok(const FExpGrantSnapshot& S)
	{
		FExpGrantOp O;
		O.bOk = true;
		O.Result = EExpGrantResult::Success;
		O.Snapshot = S;
		return O;
	}

	static FExpGrantOp Fail(FName Reason)
	{
		FExpGrantOp O;
		O.bOk = false;
		O.Result = EExpGrantResult::Rejected;
		O.ReasonTag = Reason;
		return O;
	}
};

// 11.1 이벤트 시그니처 (SSOT) :contentReference[oaicite:11]{index=11}
DECLARE_MULTICAST_DELEGATE_FourParams(FOnExpGranted, EExpSource/*Source*/, int32/*BaseExp*/, int32/*FinalExp*/, FName/*ContextDebug*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyLevelUp, int32/*NewLevel*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnExpBonusMultiplierChanged, float/*NewMultiplier*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAreaDiscovered, FName/*AreaId*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRestPointDiscovered, FName/*RestPointId*/);
