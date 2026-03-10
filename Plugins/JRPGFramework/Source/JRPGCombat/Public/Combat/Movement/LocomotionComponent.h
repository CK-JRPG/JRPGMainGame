#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "LocomotionComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

/**
 * 기본 이동(입력 기반)
 * - AddMovementInput 기반
 * - Sprint 지원
 * - InputLock(Handle)로 "플레이어 입력 이동"만 차단 가능
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API ULocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULocomotionComponent();

	// 입력(Controller에서 전달)
	void SetMoveInput(const FVector2D& InMove);     // X=Right, Y=Forward
	void SetSprint(bool bInSprint);

	// 입력 잠금(Handle)
	TJRPGResult<FJRPGHandle> AcquireInputLock(FName ReasonTag);
	FJRPGOpResult ReleaseInputLock(FJRPGHandle Handle);

	void SetMovementEnabled(bool bEnabled);
	bool CanAcceptMoveInput() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient) TObjectPtr<ACharacter> OwnerCharacter = nullptr;
	UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> CharMove = nullptr;

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bSprint = false;
	bool bMovementEnabled = true;

	uint64 NextLockHandle = 1;
	UPROPERTY(Transient) TMap<uint64, FName> ActiveLocks;

	UPROPERTY(EditAnywhere, Category="JRPG|Locomotion")
	float BaseWalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category="JRPG|Locomotion")
	float SprintMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, Category="JRPG|Locomotion")
	float InputDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, Category="JRPG|Locomotion")
	bool bUseControllerYawForMove = true;

	void CacheOwnerRefs();
	void ApplyMoveInput();
	void UpdateMoveSpeed();
};
