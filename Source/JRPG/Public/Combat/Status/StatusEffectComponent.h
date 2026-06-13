#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Status/StatusEffectDataAsset.h"
#include "StatusEffectComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStatusApplied, FName /*EffectId*/, int32 /*Stacks*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatusRemoved, FName /*EffectId*/);

UCLASS()
class UStatusModSourceObject : public UObject
{
	GENERATED_BODY()
		
public:
	UPROPERTY() 
	FName EffectId = NAME_None;
};

USTRUCT()
struct FEffectActiveStatus
{
	GENERATED_BODY()
	
	TObjectPtr<UStatusEffectDataAsset> Def = nullptr;
	TObjectPtr<UStatusModSourceObject> ModSource = nullptr;
	TWeakObjectPtr<AActor> Applier;

	int32 Stacks = 1;
	float Remaining = 0.f;
	float NextTick = 0.f;
};


UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UStatusEffectComponent :public UActorComponent
{
	GENERATED_BODY()
	
public:
	UStatusEffectComponent();

	FOnStatusApplied OnStatusApplied;
	FOnStatusRemoved OnStatusRemoved;

	// Apply/remove/query
	bool ApplyStatus(UStatusEffectDataAsset *Effect, AActor *Source, int32 AddStacks,FName ReasonTag);
	bool RemoveStatus(FName EffectId,FName ReasonTag);
	bool HasStatus(FName EffectId) const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction *ThisTickFunction) override;

private:

	UPROPERTY() 
	TArray<FEffectActiveStatus> Active;

	TWeakObjectPtr<class UCharacterCombatStatsComponent> Stats;
	TWeakObjectPtr<class UHPComponent> HP;

	int32 FindIdx(FName EffectId) const;

	void AddMods(FEffectActiveStatus &S);
	void RemoveMods(FEffectActiveStatus &S);

	void TickPeriodic(FEffectActiveStatus &S,float DeltaTime);
	void TickExpiry(float DeltaTime);
};