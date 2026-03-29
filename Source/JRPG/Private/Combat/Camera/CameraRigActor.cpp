#include "Combat/Camera/CameraRigActor.h"

#include "Camera/CameraComponent.h"
#include "Combat/Camera/CameraTargetInterface.h"
#include "GameFramework/SpringArmComponent.h"


ACameraRigActor::ACameraRigActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	RootComponent = SpringArm;
	SpringArm->TargetArmLength = DefaultArmLength;
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
	
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		Target->GetCameraTargetRotation(),
		DeltaTime,
		RotationInterpSpeed
	));
	
	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength,
		ArmLength,
		DeltaTime,
		ArmLengthInterpSpeed
	);
}

void ACameraRigActor::SetCameraTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
	
	if (NewTarget)
	{
		if (ICameraTargetInterface* T = Cast<ICameraTargetInterface>(NewTarget))
		{
			SetActorLocation(T->GetCameraTargetLocation());
			SetActorRotation(T->GetCameraTargetRotation());
			
			// 타겟 전환 시 그 타겟의 기본 ArmLength로 리셋할지 여부
			// -> 타겟마다 다르면 이 라인 살리고, 아니면 제거
			SpringArm->TargetArmLength = T->GetCameraTargetArmLength();
		}
	}
}

void ACameraRigActor::AdjustZoom(float Delta)
{
	ArmLength = FMath::Clamp(ArmLength + Delta, MinArmLength, MaxArmLength);
	UE_LOG(LogTemp, Warning, TEXT("Adjusting : Delta=%f , Zoom=%f"), Delta, ArmLength);
}

void ACameraRigActor::ResetZoom()
{
	ArmLength = DefaultArmLength;
}
