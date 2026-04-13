// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterBaseAnimInstance.generated.h"

/**
 * 
 */

class UCharacterMovementComponent;
class ACharacter;

UCLASS()
class JRPG_API UCharacterBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	void UpdateMovementVariables();

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float Speed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float Direction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float Pitch;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float Yaw;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool bFalling;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool bIsBackward;

	FVector PrevRotation;

private:
	TWeakObjectPtr<ACharacter> OwnerCharacter;
	TWeakObjectPtr<UCharacterMovementComponent> MovementComponent;



};
