#pragma once

#include "CoreMinimal.h"
#include "CombatPresentationTypes.generated.h"

UENUM()
enum class ECombatResolveTiming : uint8
{
	Immediate,
	AnimNotifyWindow,
	MontageEnded
};

UENUM()
enum class EPresentedCombatActionType : uint8
{
	None,
	BasicAttack,
	Skill,
	Item
};

USTRUCT()
struct FCombatCueEvent
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr <AActor> OwnerActor;
	UPROPERTY() FName CueTag = NAME_None;
	UPROPERTY() FName ContextTag = NAME_None;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatCueEvent, const FCombatCueEvent& /*Event*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPresentationStarted, EPresentedCombatActionType /*Type*/, FName /*ActionId*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPresentationFinished, EPresentedCombatActionType /*Type*/, FName /*ActionId*/);