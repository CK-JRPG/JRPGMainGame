#pragma once

#include "CoreMinimal.h"
#include "Combat/Camera/CameraTargetInterface.h"
#include "GameFramework/Character.h"
#include "JRPGPlayerPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ULocomotionComponent;
class ACombatCharacterActor;
class UCombatZoneTrackerComponent;

UCLASS()
class JRPG_API AJRPGPlayerPawn : public ACharacter, public ICameraTargetInterface
{
	GENERATED_BODY()

public:
	AJRPGPlayerPawn(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<ULocomotionComponent> Locomotion;
	UPROPERTY(VisibleAnywhere, Category = "JRPG")
	TObjectPtr<UCombatZoneTrackerComponent> ZoneComp;

	// ICameraTargetInterface
	virtual FVector GetCameraTargetLocation() const override;
	virtual FRotator GetCameraTargetRotation() const override;
	virtual float GetCameraTargetArmLength() const override;
	
	UPROPERTY(VisibleAnywhere, Category = "JRPG|Combat")
	FName CurrentCharacterId;
	
	UFUNCTION(BlueprintCallable, Category = "JRPG|Combat")
	void UpdateCharacter(FName NewCharId);
	
	ACombatCharacterActor* GetCombatCharData() const;
};
