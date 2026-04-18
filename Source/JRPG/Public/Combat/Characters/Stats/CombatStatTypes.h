#pragma once

#include "CoreMinimal.h"
#include "CombatStatTypes.generated.h"

UENUM()
enum class ECombatStat :uint8
{
	Attack, Defense, Speed,
	MaxHP, MaxAP, MaxSP,
	CritRate, CritDamage,
	BreakPower, HealingPower
};

UENUM()
enum class EStatModOp : uint8 { Add, Mul };

USTRUCT()
struct FCombatStatModifier
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) ECombatStat Stat = ECombatStat::Attack;
	UPROPERTY(EditAnywhere) EStatModOp Op = EStatModOp::Add;
	UPROPERTY(EditAnywhere) float Value = 0.f;

	UPROPERTY(EditAnywhere) FName Tag = NAME_None;
	UPROPERTY() TWeakObjectPtr<UObject> Source;

	static FCombatStatModifier Add(ECombatStat S, float V, FName T, UObject* Src)
	{
		FCombatStatModifier M; 
		M.Stat = S; 
		M.Op = EStatModOp::Add;
		M.Value = V;
		M.Tag = T;
		M.Source = Src;
		return M;
	}

	static FCombatStatModifier Mul(ECombatStat S, float V, FName T, UObject* Src)
	{
		FCombatStatModifier M;
		M.Stat = S;
		M.Op = EStatModOp::Mul;
		M.Value = V;
		M.Tag = T;
		M.Source = Src;
		return M;
	}
};

USTRUCT()
struct FCombatStatSnapshot
{
	GENERATED_BODY()
	UPROPERTY() float Attack = 0.f;
	UPROPERTY() float Defense = 0.f;
	UPROPERTY() float Speed = 0.f;

	UPROPERTY() float MaxHP = 100.f;
	UPROPERTY() int32 MaxAP = 10;
	UPROPERTY() int32 MaxSP = 100;

	UPROPERTY() float CritRate = 0.f;
	UPROPERTY() float CritDamage = 0.f;

	UPROPERTY() float BreakPower = 0.f;
	UPROPERTY() float HealingPower = 0.f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatStatsRecomputed, const FCombatStatSnapshot&);