#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Motion/CombatMotionTypes.h"
#include "CombatMotionComponent.generated.h"

class UCharacterMovementComponent;
class UBattleSessionSubsystem;
class UGroggyComponent;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatMotionComponent();

	FOnCombatMotionStarted OnCombatMotionStarted;
	FOnCombatMotionEnded OnCombatMotionEnded;
	FOnCombatMotionReplaced OnCombatMotionReplaced;
	FOnCombatMotionBlocked OnCombatMotionBlocked;

	FJRPGCombatMotionResponse RequestCombatMotion(const FJRPGCombatMotionRequest& Req);
	bool CancelCombatMotion(FJRPGCombatMotionHandle Handle, FName ReasonTag);
	int32 CancelAllByOwner(FName OwnerTag, FName ReasonTag);
 
	bool IsMotionActive() const { return MotionState.ActiveHandle.IsValid(); }
	EJRPGCombatMotionType GetActiveMotionType() const { return MotionState.ActiveRequest.Type; }
	const FCombatMotionState& GetMotionState() const { return MotionState; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()FCombatMotionState MotionState;
	uint64 NextMotionId = 1;
	double LastHitMoveRequestRealSec = -1000.0;

	TWeakObjectPtr<UCharacterMovementComponent> CharMove;
	TWeakObjectPtr<UGroggyComponent> Groggy;

	UBattleSessionSubsystem* GetBattle() const;
	ACharacter* GetCharacterOwner() const;


	int32 GetPriorityForType(EJRPGCombatMotionType Type) const;
	bool CanReplaceCurrent(const FJRPGCombatMotionRequest& NewReq, FName& OutReason) const;
 
	bool ResolveRequestContext(FJRPGCombatMotionRequest& InOutReq, FName& OutReason);
	bool ValidateRequest(const FJRPGCombatMotionRequest& Req, FName& OutReason) const;
 
	void StartAcceptedMotion(const FJRPGCombatMotionRequest& Req, const FJRPGCombatMotionHandle& Handle, bool bBroadcastReplaced, const FJRPGCombatMotionHandle& ReplacedHandle);
	void EndActiveMotion(FName EndReasonTag);
 
	void TickVelocityCurve(float DeltaTime);
	void TickRootMotion(float DeltaTime);
	void ApplyClampIfNeeded();
	void FinishTeleportRequestImmediately(const FJRPGCombatMotionRequest& Req, const FJRPGCombatMotionHandle& Handle);

	FVector MakeDirectionFromTarget(AActor* TargetActor) const;
	FVector ClampLocationToBattle(const FVector& InLocation, bool& bWasClamped) const;
};
