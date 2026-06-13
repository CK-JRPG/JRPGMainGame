#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FieldCharacterAnimInstance.generated.h"

class UCharacterMovementComponent;
class ULocomotionComponent;

/*
 - 필드 캐릭터용 AnimInstance 베이스 클래스
 - AJRPGPlayerPawn / AJRPGCompanionPawn 공용
 - LocomotionComponent 기반 이동 속성을 캐싱하여 ABP에 노출
 */

UCLASS()
class JRPG_API UFieldCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<ACharacter> OwningCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCharacterMovementComponent> MovementComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<ULocomotionComponent> LocomotionComp = nullptr;


	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float MovementDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FVector2D MoveInput = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	float MoveSpeedThreshold = 3.f;
};
