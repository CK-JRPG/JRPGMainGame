#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JRPGPlayerPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ULocomotionComponent;
class UCombatZoneTrackerComponent;

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
	
	// 카메라
	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<UCameraComponent> FollowCamera;
};
