#include "Combat/Camera/CameraRigActor.h"

#include "Camera/CameraComponent.h"
#include "Combat/Camera/CameraTargetInterface.h"
#include "GameFramework/SpringArmComponent.h"


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
}

void ACameraRigActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
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
