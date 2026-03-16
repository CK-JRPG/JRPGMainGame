#pragma once

#include "CoreMinimal.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "GameFramework/Character.h"
#include "JRPGPlayerPawn.generated.h"

class ULocomotionComponent;
class UCombatZoneTrackerComponent;
class ACombatCharacterActor;

UCLASS()
class JRPG_API AJRPGPlayerPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AJRPGPlayerPawn(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<ULocomotionComponent> Locomotion;

	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<UCombatZoneTrackerComponent> ZoneTracker;
};
