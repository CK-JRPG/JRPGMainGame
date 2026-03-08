#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Status/StatusEffectDataAsset.h"
#include "StatusEffectComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStatusApplied, FName /*EffectId*/, int32 /*Stacks*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatusRemoved, FName /*EffectId*/);

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
	UCLASS()
	class UStatusModSourceObject : public UObject
	{
		GENERATED_BODY()
		
	public:
		UPROPERTY() 
		FName EffectId = NAME_None;
	};

	struct FActiveStatus
	{
		TObjectPtr<UStatusEffectDataAsset> Def = nullptr;
		TObjectPtr<UStatusModSourceObject> ModSource = nullptr;
		TWeakObjectPtr<AActor> Applier;

		int32 Stacks = 1;
		float Remaining = 0.f;
		float NextTick = 0.f;
	};

	UPROPERTY() 
	TArray<FActiveStatus> Active;

	TWeakObjectPtr<class UCombatStatsComponent> Stats;
	TWeakObjectPtr<class UCombatHPComponent> HP;

	int32 FindIdx(FName EffectId) const;

	void AddMods(FActiveStatus &S);
	void RemoveMods(FActiveStatus &S);

	void TickPeriodic(FActiveStatus &S,float DeltaTime);
	void TickExpiry(float DeltaTime);
};