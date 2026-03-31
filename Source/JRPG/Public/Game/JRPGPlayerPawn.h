#pragma once

#include "CoreMinimal.h"
#include "Combat/Camera/CameraTargetInterface.h"
#include "GameFramework/Character.h"
#include "JRPGPlayerPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ULocomotionComponent;
class ACombatCharacterActor;

UCLASS()
class JRPG_API AJRPGPlayerPawn : public ACharacter, public ICameraTargetInterface
{
	GENERATED_BODY()

public:
	AJRPGPlayerPawn(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<ULocomotionComponent> Locomotion;

	// ICameraTargetInterface
	virtual FVector GetCameraTargetLocation() const override;
	virtual FRotator GetCameraTargetRotation() const override;
	virtual float GetCameraTargetArmLength() const override;
	
	// 해당 접근 방식이 맞는지에 대해서 점검중.(CharacterId로 CombatCharacterActor 찾아서 브릿지 역할 하는 방식)
	UPROPERTY(VisibleAnywhere, Category = "JRPG|Combat")
	FName CurrentCharacterId;
	
	UFUNCTION(BlueprintCallable, Category = "JRPG|Combat")
	void UpdateCharacter(FName NewCharId);
	
	ACombatCharacterActor* GetCombatCharData() const;
};
