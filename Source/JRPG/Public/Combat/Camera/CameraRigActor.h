#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraRigActor.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class JRPG_API ACameraRigActor : public AActor
{
	GENERATED_BODY()

public:
	ACameraRigActor();
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;
	
	// UCameraSubsystem만 호출
	void SetCameraTarget(AActor* NewTarget);
	AActor* GetCurrentTarget() const { return TargetActor.Get(); }
	
	// 타겟만 변경하고 카메라 위치/회전은 보간으로 전환 (끊김 없는 전환용)
	void SetCameraTargetSmooth(AActor* NewTarget);
	
	// 락온
	void SetLockOnTarget(AActor* Target);
	void ClearLockOnTarget();
	bool HasLockOnTarget() const { return LockOnTarget.IsValid(); }

	// Getter/Setter
	float GetLocationInterpSpeed() const { return LocationInterpSpeed; }
	float GetRotationInterpSpeed() const { return RotationInterpSpeed; }
	float GetArmLengthInterpSpeed() const { return ArmLengthInterpSpeed; }
	float GetZoomStep() const { return ZoomStep; }

public:
	void AdjustZoom(float Delta);
	void ResetZoom();
	void SetArmLength(float NewArmLength, bool bApplyImmediately = true);
	void UseFieldArmLength(bool bApplyImmediately = false);
	void UseCombatArmLength(bool bApplyImmediately = false);

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> LockOnTarget;
	
	// 보간 속도
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float LocationInterpSpeed = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float RotationInterpSpeed = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float ArmLengthInterpSpeed = 5.0f;
	
	// 카메라 설정
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float ZoomStep = 80.0f;
	
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "100.0"))
	float MinArmLength = 150.0f;
	
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "150.0"))
	float FieldArmLength = 550.0f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "100.0"))
	float FieldMaxArmLength = 550.0f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "150.0"))
	float CombatArmLength = 950.0f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "100.0"))
	float CombatMaxArmLength = 950.0f;

	float CurrentArmLength = 550.0f;
	float CurrentDefaultArmLength = 550.0f;
	float CurrentMaxArmLength = 550.0f;
	
	// 락온 시 적의 위치에 더할 수직 오프셋 (가슴/머리 높이)
	float LockOnVerticalOffset = 60.0f;

	bool bLockRotation = false;
};
