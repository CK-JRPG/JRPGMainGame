#include "Combat/Camera/CameraRigActor.h"

#include "Camera/CameraComponent.h"
#include "Combat/Camera/CameraTargetInterface.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInterface.h"


ACameraRigActor::ACameraRigActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	RootComponent = SpringArm;
	SpringArm->TargetArmLength = FieldArmLength;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bDoCollisionTest = true; // 카메라 콜리전 설정해줘야 함. 플레이어어와 다른 객체라서 부딪힘.
	
	SpringArm->ProbeChannel = ECC_Visibility;
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	TargetOutlinePostProcessMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Combat/Presentation/M_CombatTargetOutline_PP.M_CombatTargetOutline_PP")));
}

void ACameraRigActor::BeginPlay()
{
	Super::BeginPlay();

	if (Camera)
	{
		CameraDefaultRelativeLocation = Camera->GetRelativeLocation();
	}

	ApplyTargetOutlinePostProcess();
}

void ACameraRigActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateManualCameraShake(DeltaTime);
	
	if (!TargetActor.IsValid()) return;
	
	ICameraTargetInterface* Target = Cast<ICameraTargetInterface>(TargetActor.Get());
	if (!Target) return;
	
	SetActorLocation(FMath::VInterpTo(
		GetActorLocation(),
		Target->GetCameraTargetLocation(),
		DeltaTime,
		LocationInterpSpeed
	));

	FRotator DesiredRotation;
	if (LockOnTarget.IsValid())
	{
		const FVector MyLoc = GetActorLocation();
		// 적의 발 위치보다 약간 위쪽을 바라보게 오프셋 설정함
		const FVector EnemyLoc = LockOnTarget->GetActorLocation() + FVector(0.f, 0.f, LockOnVerticalOffset);
		DesiredRotation = (EnemyLoc - MyLoc).Rotation();
	}
	else
	{
		DesiredRotation = Target->GetCameraTargetRotation();
	}
	
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		DesiredRotation,
		DeltaTime,
		RotationInterpSpeed
	));
	
	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength,
		CurrentArmLength,
		DeltaTime,
		ArmLengthInterpSpeed
	);
}

void ACameraRigActor::SetCameraTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void ACameraRigActor::AdjustZoom(float Delta)
{
	SetArmLength(CurrentArmLength + Delta, false);
	UE_LOG(LogTemp, Warning, TEXT("Adjusting : Delta=%f , Zoom=%f"), Delta, CurrentArmLength);
}

void ACameraRigActor::ResetZoom()
{
	SetArmLength(CurrentDefaultArmLength, false);
}

void ACameraRigActor::SetCameraTargetSmooth(AActor* NewTarget)
{
	TargetActor = NewTarget;
	// 위치/회전/ArmLength는 Tick에서 보간으로 자연스럽게 전환
}

void ACameraRigActor::SetLockOnTarget(AActor* Target)
{
	LockOnTarget = Target;
}

void ACameraRigActor::ClearLockOnTarget()
{
	LockOnTarget.Reset();
}

void ACameraRigActor::SetArmLength(float NewArmLength, bool bApplyImmediately)
{
	CurrentArmLength = FMath::Clamp(NewArmLength, MinArmLength, CurrentMaxArmLength);
	if (bApplyImmediately && SpringArm)
	{
		SpringArm->TargetArmLength = CurrentArmLength;
	}
}

void ACameraRigActor::UseFieldArmLength(bool bApplyImmediately)
{
	CurrentDefaultArmLength = FieldArmLength;
	CurrentMaxArmLength = FieldMaxArmLength;
	SetArmLength(FieldArmLength, bApplyImmediately);
}

void ACameraRigActor::UseCombatArmLength(bool bApplyImmediately)
{
	CurrentDefaultArmLength = CombatArmLength;
	CurrentMaxArmLength = CombatMaxArmLength;
	SetArmLength(CombatArmLength, bApplyImmediately);
}

void ACameraRigActor::PlayCombatCameraShake(const FCombatCameraShakeSpec& ShakeSpec, bool bCriticalHit)
{
	if (!ShakeSpec.bEnable)
	{
		return;
	}

	StartManualCameraShake(ShakeSpec, bCriticalHit);

	if (!ShakeSpec.CameraShakeClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	const float Scale = bCriticalHit ? ShakeSpec.CriticalCameraShakeScale : ShakeSpec.CameraShakeScale;
	if (Scale > 0.f)
	{
		PC->PlayerCameraManager->StartCameraShake(ShakeSpec.CameraShakeClass, Scale);
	}
}

void ACameraRigActor::StartManualCameraShake(const FCombatCameraShakeSpec& ShakeSpec, bool bCriticalHit)
{
	if (!ShakeSpec.bUseManualShake || !Camera)
	{
		return;
	}

	const float CriticalMultiplier = bCriticalHit ? ShakeSpec.ManualCriticalMultiplier : 1.0f;
	ManualShakeElapsed = 0.0f;
	ActiveManualShakeDuration = FMath::Max(0.01f, ShakeSpec.ManualDuration);
	ActiveManualShakeHorizontalAmplitude = ShakeSpec.ManualHorizontalAmplitude * CriticalMultiplier;
	ActiveManualShakeVerticalAmplitude = ShakeSpec.ManualVerticalAmplitude * CriticalMultiplier;
	ActiveManualShakeSpeed = ShakeSpec.ManualSpeed;
	bManualShakeActive = true;
}

void ACameraRigActor::UpdateManualCameraShake(float DeltaTime)
{
	if (!bManualShakeActive || !Camera)
	{
		return;
	}

	ManualShakeElapsed += DeltaTime;
	const float NormalizedTime = ManualShakeElapsed / ActiveManualShakeDuration;
	if (NormalizedTime >= 1.0f)
	{
		bManualShakeActive = false;
		Camera->SetRelativeLocation(CameraDefaultRelativeLocation);
		return;
	}

	const float Decay = FMath::Square(1.0f - NormalizedTime);
	const float Phase = ManualShakeElapsed * ActiveManualShakeSpeed;
	const float Horizontal = FMath::Sin(Phase) * ActiveManualShakeHorizontalAmplitude * Decay;
	const float Vertical = FMath::Cos(Phase * 1.37f) * ActiveManualShakeVerticalAmplitude * Decay;

	Camera->SetRelativeLocation(CameraDefaultRelativeLocation + FVector(0.0f, Horizontal, Vertical));
}

void ACameraRigActor::ApplyTargetOutlinePostProcess()
{
	if (!Camera || TargetOutlinePostProcessMaterial.IsNull() || TargetOutlinePostProcessWeight <= 0.0f)
	{
		return;
	}

	UMaterialInterface* OutlineMaterial = TargetOutlinePostProcessMaterial.LoadSynchronous();
	if (!OutlineMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraRigActor: target outline post-process material not found: %s"),
			*TargetOutlinePostProcessMaterial.ToString());
		return;
	}

	Camera->PostProcessSettings.AddBlendable(OutlineMaterial, TargetOutlinePostProcessWeight);
}
