#include "Combat/Movement/LocomotionComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

ULocomotionComponent::ULocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerRefs();
	UpdateMoveSpeed();
}

void ULocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveLocks.Reset();
	Super::EndPlay(EndPlayReason);
}

void ULocomotionComponent::CacheOwnerRefs()
{
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("[Locomotion] Owner must be ACharacter."));
		return;
	}

	CharMove = OwnerCharacter->GetCharacterMovement();
	if (!CharMove)
	{
		UE_LOG(LogTemp, Error, TEXT("[Locomotion] CharacterMovementComponent missing."));
	}
}

void ULocomotionComponent::SetMoveInput(const FVector2D& InMove)
{
	MoveInput = InMove;
}

void ULocomotionComponent::SetSprint(bool bInSprint)
{
	if (bSprint == bInSprint) return;
	bSprint = bInSprint;
	UpdateMoveSpeed();
}

TJRPGResult<FJRPGHandle> ULocomotionComponent::AcquireInputLock(FName ReasonTag)
{
	FJRPGHandle H;
	H.Value = NextLockHandle++;
	ActiveLocks.Add(H.Value, ReasonTag.IsNone() ? FName("Locomotion.Lock") : ReasonTag);
	return TJRPGResult<FJRPGHandle>::Ok(H);
}

FJRPGOpResult ULocomotionComponent::ReleaseInputLock(FJRPGHandle Handle)
{
	if (!Handle.IsValid())
	{
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Locomotion.HandleInvalid"));
	}
	const int32 Removed = ActiveLocks.Remove(Handle.Value);
	if (Removed <= 0)
	{
		return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Locomotion.HandleNotFound"));
	}
	return FJRPGOpResult::Ok();
}

void ULocomotionComponent::SetMovementEnabled(bool bEnabled)
{
	bMovementEnabled = bEnabled;
}

bool ULocomotionComponent::CanAcceptMoveInput() const
{
	if (!bMovementEnabled) return false;
	if (!OwnerCharacter || !CharMove) return false;
	if (ActiveLocks.Num() > 0) return false; // 플레이어 입력만 차단
	return true;
}

void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || !CharMove) return;
	ApplyMoveInput();
}

void ULocomotionComponent::ApplyMoveInput()
{
	if (!CanAcceptMoveInput())
	{
		bMoveBasisYawLocked = false;
		return;
	}
 
	const bool bHasMoveInput = FMath::Abs(MoveInput.X) >= InputDeadZone || FMath::Abs(MoveInput.Y) >= InputDeadZone;
	if (!bHasMoveInput)
	{
		bMoveBasisYawLocked = false;
		return;
	}
 
	AController* C = OwnerCharacter->GetController();
	if (!C)
	{
		OwnerCharacter->AddMovementInput(FVector::ForwardVector, MoveInput.Y);
		OwnerCharacter->AddMovementInput(FVector::RightVector, MoveInput.X);
		return;
	}
 
	FRotator BasisRot = FRotator::ZeroRotator;
	if (bUseControllerYawForMove)
	{
		if (!bMoveBasisYawLocked)
		{
			const FRotator ControlRot = C->GetControlRotation();
			LockedMoveBasisYaw = ControlRot.Yaw;
			bMoveBasisYawLocked = true;
		}
		BasisRot = FRotator(0.f, LockedMoveBasisYaw, 0.f);
	}
 
	const FVector Forward = FRotationMatrix(BasisRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(BasisRot).GetUnitAxis(EAxis::Y);
 
	OwnerCharacter->AddMovementInput(Forward, MoveInput.Y);
	OwnerCharacter->AddMovementInput(Right, MoveInput.X);
}

void ULocomotionComponent::UpdateMoveSpeed()
{
	if (!CharMove) return;
	const float Target = BaseWalkSpeed * (bSprint ? SprintMultiplier : 1.f);
	CharMove->MaxWalkSpeed = Target;
}
