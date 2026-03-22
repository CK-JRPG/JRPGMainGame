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
	
private:
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;
	
	// 보간 속도
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float LocationInterpSpeed = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float RotationInterpSpeed = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float ArmLengthInterpSpeed = 5.0f;
	
	bool bLockRotation = false;
	
public:
	float GetLocationInterpSpeed() const { return LocationInterpSpeed; }
	float GetRotationInterpSpeed() const { return RotationInterpSpeed; }
	float GetArmLengthInterpSpeed() const { return ArmLengthInterpSpeed; }
};
