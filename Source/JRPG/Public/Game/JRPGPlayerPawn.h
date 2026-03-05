#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JRPGPlayerPawn.generated.h"

class ULocomotionComponent;
class UCombatZoneTrackerComponent;

UCLASS()
class PROJECTGAME_API AJRPGPlayerPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AJRPGPlayerPawn(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<ULocomotionComponent> Locomotion;

	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<UCombatZoneTrackerComponent> ZoneTracker;
};
