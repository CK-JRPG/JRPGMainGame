#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraRigActor.generated.h"

class UCameraComponent;
class UCameraShakeBase;
class USpringArmComponent;

UCLASS()
class JRPG_API ACameraRigActor : public AActor
{
	GENERATED_BODY()

public:
	ACameraRigActor();
	
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

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
	void PlayPlayerHitCameraShake(float DamageAmount, bool bCriticalHit);

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

	// 추가 CameraShakeBase 에셋을 함께 재생함. None이면 아래 수동 쉐이크만 적용됨.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> PlayerHitCameraShakeClass;

	// PlayerHitCameraShakeClass 재생 강도. 값이 클수록 에셋 기반 쉐이크가 강해짐.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake", meta = (ClampMin = "0.0"))
	float PlayerHitCameraShakeScale = 0.75f;

	// 크리티컬 히트일 때 PlayerHitCameraShakeClass 재생 강도.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake", meta = (ClampMin = "0.0"))
	float PlayerCriticalHitCameraShakeScale = 1.25f;

	// true면 카메라 컴포넌트를 직접 좌우/상하로 흔듦. CameraShakeBase 에셋 없이도 동작함.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake")
	bool bUseManualPlayerHitCameraShake = true;

	// 좌우 흔들림 폭. 값이 클수록 화면이 좌우로 크게 흔들림.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake", meta = (ClampMin = "0.0", EditCondition = "bUseManualPlayerHitCameraShake"))
	float PlayerHitShakeHorizontalAmplitude = 12.0f;

	// 상하 흔들림 폭. 값이 클수록 화면이 위아래로 크게 흔들림.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake", meta = (ClampMin = "0.0", EditCondition = "bUseManualPlayerHitCameraShake"))
	float PlayerHitShakeVerticalAmplitude = 8.0f;

	// 흔들림 지속 시간(초). 값이 클수록 쉐이크가 오래 남음.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake", meta = (ClampMin = "0.01", EditCondition = "bUseManualPlayerHitCameraShake"))
	float PlayerHitShakeDuration = 0.18f;

	// 흔들림 왕복 속도. 값이 클수록 더 빠르게 떨림.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake", meta = (ClampMin = "0.01", EditCondition = "bUseManualPlayerHitCameraShake"))
	float PlayerHitShakeSpeed = 32.0f;

	// 크리티컬 히트일 때 수동 쉐이크 폭 배율. 1.0이면 일반 히트와 동일함.
	UPROPERTY(EditAnywhere, Category = "Camera|Shake", meta = (ClampMin = "0.0", EditCondition = "bUseManualPlayerHitCameraShake"))
	float PlayerHitShakeCriticalMultiplier = 1.4f;

	float CurrentArmLength = 550.0f;
	float CurrentDefaultArmLength = 550.0f;
	float CurrentMaxArmLength = 550.0f;
	
	// 락온 시 적의 위치에 더할 수직 오프셋 (가슴/머리 높이)
	float LockOnVerticalOffset = 60.0f;

	bool bLockRotation = false;

	// 수동 쉐이크 종료 후 카메라 컴포넌트를 원래 상대 위치로 되돌리기 위한 런타임 값.
	FVector CameraDefaultRelativeLocation = FVector::ZeroVector;
	float PlayerHitShakeElapsed = 0.0f;
	float ActivePlayerHitShakeDuration = 0.0f;
	float ActivePlayerHitShakeHorizontalAmplitude = 0.0f;
	float ActivePlayerHitShakeVerticalAmplitude = 0.0f;
	bool bPlayerHitShakeActive = false;

	void StartManualPlayerHitCameraShake(bool bCriticalHit);
	void UpdateManualPlayerHitCameraShake(float DeltaTime);
};
