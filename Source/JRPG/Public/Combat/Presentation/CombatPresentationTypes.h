#pragma once

#include "CoreMinimal.h"
#include "CombatPresentationTypes.generated.h"

class UCameraShakeBase;

UENUM()
enum class ECombatResolveTiming : uint8
{
	Immediate,
	AnimNotifyWindow,
	MontageEnded
};

UENUM()
enum class EPresentedCombatActionType : uint8
{
	None,
	BasicAttack,
	Skill,
	Item
};

USTRUCT(BlueprintType)
struct FCombatCameraShakeSpec
{
	GENERATED_BODY()

	// 쉐이크 켜짐 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake")
	bool bEnable = false;

	// 추가 CameraShakeBase 에셋을 함께 재생함. None이면 아래 수동 쉐이크만 적용됨.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(EditCondition="bEnable"))
	TSubclassOf<UCameraShakeBase> CameraShakeClass = nullptr;

	// CameraShakeClass 재생 강도. 값이 클수록 에셋 기반 쉐이크가 강해짐.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(ClampMin="0.0", EditCondition="bEnable"))
	float CameraShakeScale = 0.75f;
	
	// 크리티컬 히트일 때 CameraShakeClass 재생 강도.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(ClampMin="0.0", EditCondition="bEnable"))
	float CriticalCameraShakeScale = 1.25f;

	// true면 카메라 컴포넌트를 직접 좌우/상하로 흔듦. CameraShakeBase 에셋 없이도 동작함.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(EditCondition="bEnable"))
	bool bUseManualShake = true;

	// 좌우 흔들림 폭. 값이 클수록 화면이 좌우로 크게 흔들림.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(ClampMin="0.0", EditCondition="bEnable && bUseManualShake"))
	float ManualHorizontalAmplitude = 12.0f;

	// 상하 흔들림 폭. 값이 클수록 화면이 위아래로 크게 흔들림.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(ClampMin="0.0", EditCondition="bEnable && bUseManualShake"))
	float ManualVerticalAmplitude = 8.0f;

	// 흔들림 지속 시간(초). 값이 클수록 쉐이크가 오래 남음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(ClampMin="0.01", EditCondition="bEnable && bUseManualShake"))
	float ManualDuration = 0.18f;

	// 흔들림 왕복 속도. 값이 클수록 더 빠르게 떨림.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(ClampMin="0.01", EditCondition="bEnable && bUseManualShake"))
	float ManualSpeed = 32.0f;

	// 크리티컬 히트일 때 수동 쉐이크 폭 배율. 1.0이면 일반 히트와 동일함.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Shake", meta=(ClampMin="0.0", EditCondition="bEnable && bUseManualShake"))
	float ManualCriticalMultiplier = 1.4f;
};

USTRUCT()
struct FCombatCueEvent
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr <AActor> OwnerActor;
	UPROPERTY() FName CueTag = NAME_None;
	UPROPERTY() FName ContextTag = NAME_None;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatCueEvent, const FCombatCueEvent& /*Event*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPresentationStarted, EPresentedCombatActionType /*Type*/, FName /*ActionId*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPresentationFinished, EPresentedCombatActionType /*Type*/, FName /*ActionId*/);
