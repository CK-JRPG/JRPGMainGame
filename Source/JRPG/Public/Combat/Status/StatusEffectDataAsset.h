#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Characters/Stats/CombatStatTypes.h"
#include "Combat/Status/StatusEffectTypes.h"
#include "StatusEffectDataAsset.generated.h"

UCLASS()
class JRPG_API UStatusEffectDataAsset :public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere) FName EffectId = NAME_None;

	UPROPERTY(EditAnywhere) float DurationSec = 5.f;
	UPROPERTY(EditAnywhere) int32 MaxStacks = 1;
	UPROPERTY(EditAnywhere) EJRPGStatusStackPolicy StackPolicy = EJRPGStatusStackPolicy::RefreshDuration;
	
	UPROPERTY(EditAnywhere) FPeriodicEffect Periodic;

	UPROPERTY(EditAnywhere) TArray<FCombatStatModifier> StatMods;// Source는 런타임에서 교체

	bool IsValidDef() const { return!EffectId.IsNone(); }
};