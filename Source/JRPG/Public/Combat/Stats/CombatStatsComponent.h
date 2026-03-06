// Source/JRPGCombat/Public/Combat/Stats/CombatStatsComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Items/ItemModifierTypes.h"
#include "CombatStatsComponent.generated.h"

USTRUCT()
struct FCombatBaseStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Attack = 10.f;
	UPROPERTY(EditAnywhere)
	float Defense = 5.f;
	UPROPERTY(EditAnywhere)
	float MaxHP = 100.f;

	UPROPERTY(EditAnywhere)
	float BreakPower = 1.0f;
	UPROPERTY(EditAnywhere)
	float HealingPower = 1.0f;
	UPROPERTY(EditAnywhere)
	float ThreatMod = 1.0f;
};

USTRUCT()
struct FCombatFinalStats
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	float Attack = 10.f;
	UPROPERTY(VisibleAnywhere)
	float Defense = 5.f;
	UPROPERTY(VisibleAnywhere)
	float MaxHP = 100.f;

	UPROPERTY(VisibleAnywhere)
	float BreakPower = 1.0f;
	UPROPERTY(VisibleAnywhere)
	float HealingPower = 1.0f;
	UPROPERTY(VisibleAnywhere)
	float ThreatMod = 1.0f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFinalStatsChanged, const FCombatFinalStats&);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPG_API UCombatStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FCombatBaseStats Base;
	UPROPERTY(VisibleAnywhere)
	FCombatFinalStats Final;

	FOnFinalStatsChanged OnFinalStatsChanged;

	void ApplyAugmentModifierSet(const FAugmentModifierSet& Mods);
	const FCombatFinalStats& GetFinal() const { return Final; }

private:
	static float ComputeFinal(float BaseValue, const FStatModifierAccumulator& A);
};
