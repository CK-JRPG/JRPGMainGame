// Source/JRPGCombat/Public/Combat/Progression/Bond/BondTypes.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BondTypes.generated.h"

// 문서 API: AddBondPoints(Source, Participants, BaseAmount, Context) :contentReference[oaicite:7]{index=7}
UENUM()
enum class EBondSource : uint8
{
	Walk,
	CombatWin,
	RestTalk,
	DialogueBonus
};

UENUM()
enum class EBondResult : uint8
{
	Success,
	Rejected
};

USTRUCT()
struct FBondOp
{
	GENERATED_BODY()

	UPROPERTY()
	bool bOk = false;
	
	UPROPERTY()
	EBondResult Result = EBondResult::Rejected;
	
	UPROPERTY()
	FName ReasonTag = NAME_None;

	static FBondOp Ok()
	{
		FBondOp O;
		O.bOk = true;
		O.Result = EBondResult::Success;
		return O;
	}

	static FBondOp Fail(FName Reason)
	{
		FBondOp O;
		O.bOk = false;
		O.Result = EBondResult::Rejected;
		O.ReasonTag = Reason;
		return O;
	}
};

// 7.1 Bond 대상 ID (SSOT) :contentReference[oaicite:8]{index=8}
USTRUCT()
struct FBondPairId
{
	GENERATED_BODY()

	UPROPERTY() FName A = NAME_None;
	UPROPERTY() FName B = NAME_None;

	static FBondPairId Make(FName InA, FName InB)
	{
		FBondPairId Id;
		if (InA.ToString() <= InB.ToString())
		{
			Id.A = InA;
			Id.B = InB;
		}
		else
		{
			Id.A = InB;
			Id.B = InA;
		}
		return Id;
	}

	bool IsValid() const { return !A.IsNone() && !B.IsNone() && A != B; }
	FName ToDebugName() const { return FName(*(A.ToString() + TEXT("_") + B.ToString())); }

	bool operator==(const FBondPairId& R) const { return A == R.A && B == R.B; }
};

FORCEINLINE uint32 GetTypeHash(const FBondPairId& K)
{
	return HashCombineFast(GetTypeHash(K.A), GetTypeHash(K.B));
}

USTRUCT()
struct FBondTrioId
{
	GENERATED_BODY()

	UPROPERTY() FName A = NAME_None;
	UPROPERTY() FName B = NAME_None;
	UPROPERTY() FName C = NAME_None;

	static FBondTrioId Make(FName X, FName Y, FName Z)
	{
		TArray<FName> T{X, Y, Z};
		T.Sort([](const FName& L, const FName& R) { return L.ToString() < R.ToString(); });

		FBondTrioId Id;
		Id.A = T[0];
		Id.B = T[1];
		Id.C = T[2];
		return Id;
	}

	bool IsValid() const
	{
		return !A.IsNone() && !B.IsNone() && !C.IsNone() && A != B && B != C && A != C;
	}

	FName ToDebugName() const { return FName(*(A.ToString() + TEXT("_") + B.ToString() + TEXT("_") + C.ToString())); }

	bool operator==(const FBondTrioId& R) const { return A == R.A && B == R.B && C == R.C; }
};

FORCEINLINE uint32 GetTypeHash(const FBondTrioId& K)
{
	return HashCombineFast(HashCombineFast(GetTypeHash(K.A), GetTypeHash(K.B)), GetTypeHash(K.C));
}

// 7.2 Bond 상태 (SSOT) :contentReference[oaicite:9]{index=9}
USTRUCT()
struct FBondState
{
	GENERATED_BODY()

	UPROPERTY() int32 BondLevel = 1; // 1~5
	UPROPERTY() int32 BondPoint = 0; // 권장 0~99, 레벨업 시 -100
	UPROPERTY() int32 TotalEarnedBP = 0;
	UPROPERTY() double LastBPEventTimeReal = 0.0; // 악용 방지용

	static FBondState Default()
	{
		FBondState S;
		S.BondLevel = 1;
		S.BondPoint = 0;
		S.TotalEarnedBP = 0;
		S.LastBPEventTimeReal = 0.0;
		return S;
	}
};

// 이벤트 시그니처 (SSOT) :contentReference[oaicite:10]{index=10}
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBondPointsGained, FName/*BondId*/, int32/*Amount*/, FName/*SourceTag*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBondLevelUp, FName/*BondId*/, int32/*NewLevel*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBondDialogueUnlocked, FName/*NodeId*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBondDialogueCompleted, FName/*NodeId*/, FName/*BondId*/, int32/*RewardBP*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBondExpBonusChanged, float/*NewMultiplier*/);
