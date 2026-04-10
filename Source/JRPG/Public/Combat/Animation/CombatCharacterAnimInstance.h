#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Combat/Motion/CombatMotionTypes.h"
#include "CombatCharacterAnimInstance.generated.h"

class ACombatCharacterActor;
class UCharacterMovementComponent;
class UCombatMotionComponent;
class UGroggyComponent;
class UStatusEffectComponent;
class UCombatActionComponent;
class ULocomotionComponent;
class UHPComponent;

/*
- 전투 캐릭터용 AnimInstance 베이스 클래스
- ACombatCharacterActor 전용
- 전투 컴포넌트(Groggy, Motion, HP 등)를 캐싱하여 ABP에 노출
 */
UCLASS()
class JRPG_API UCombatCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<ACombatCharacterActor> OwningCombatActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCharacterMovementComponent> MovementComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCombatMotionComponent> MotionComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UGroggyComponent> GroggyComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UStatusEffectComponent> StatusComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCombatActionComponent> ActionComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<ULocomotionComponent> LocomotionComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UHPComponent> HPComp = nullptr;


	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float MovementDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAccelerating = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsGroggy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsCombatMotionActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EJRPGCombatMotionType ActiveMotionType = EJRPGCombatMotionType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDead = false;
};
