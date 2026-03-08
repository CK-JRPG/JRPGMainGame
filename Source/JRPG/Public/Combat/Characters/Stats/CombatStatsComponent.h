#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Characters/Stats/CombatStatTypes.h"
#include "CombatStatsComponent.generated.h"

class UCombatCharacterComponent;
class UCombatHPComponent;
class UCombatAPComponent;
class USPComponent;

USTRUCT()
struct FLevelScalingConfig
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float AttackPerLevelMul = 0.04f;
	UPROPERTY(EditAnywhere) float DefensePerLevelMul = 0.03f;
	UPROPERTY(EditAnywhere) float HPPerLevelMul = 0.05f;
	UPROPERTY(EditAnywhere) float SpeedPerLevelMul = 0.01f;

	float MulByLevel(float PerLevel, int32 Level) const
	{
		const int32 L = FMath::Max(1, Level);
		return 1.0f + PerLevel * (float)(L - 1);
	}
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatStatsComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UCombatStatsComponent();

	UPROPERTY(EditAnywhere) FLevelScalingConfig LevelScaling;
	FOnCombatStatsRecomputed OnCombatStatsRecomputed;

	void AddModifier(const FCombatStatModifier& Mod);
	void RemoveModifiersBySource(UObject* Source);
	void ClearModifiers();

	const FCombatStatSnapshot& GetSnapshot() const { return Snapshot; }
	float GetStatFloat(ECombatStat Stat) const;

	void RecomputeStats(FName ReasonTag);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TArray<FCombatStatModifier> Mods;
	UPROPERTY() FCombatStatSnapshot Snapshot;

	TWeakObjectPtr<UCombatCharacterComponent> CharacterComp;
	TWeakObjectPtr<UCombatHPComponent> HP;
	TWeakObjectPtr<UCombatAPComponent> AP;
	TWeakObjectPtr<USPComponent> SP;

	int32 QueryPartyLevel() const;

	void ApplyMods(ECombatStat Stat, float& InOutValue) const;
	void ApplyToResources(bool bKeepHPRatio);
};
