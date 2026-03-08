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

	FCombatMotionResponse RequestCombatMotion(const FCombatMotionRequest& Req);
	bool CancelCombatMotion(FCombatMotionHandle Handle, FName ReasonTag);
	int32 CancelAllByOwner(FName OwnerTag,FName ReasonTag);

	bool IsMotionActive() const { return MotionState.ActiveHandle.IsValid(); }
	ECombatMotionType GetActiveMotionType() const { return MotionState.ActiveRequest.Type; }
	constFCombatMotionState& GetMotionState() const { return MotionState; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()FCombatMotionState MotionState;
	uint64 NextMotionId = 1;

	TWeakObjectPtr<UCharacterMovementComponent> CharMove;
	TWeakObjectPtr<UGroggyComponent> Groggy;

	UBattleSessionSubsystem* GetBattle() const;
	ACharacter* GetCharacterOwner() const;

	int32 GetPriorityForType(ECombatMotionType Type) const;
	bool CanReplaceCurrent(const FCombatMotionRequest& NewReq, FName& OutReason) const;

	bool ResolveRequestContext(FCombatMotionRequest& InOutReq, FName& OutReason);
	bool ValidateRequest(const FCombatMotionRequest& Req, FName& OutReason) const;

	void StartAcceptedMotion(const FCombatMotionRequest& Req, const FCombatMotionHandle& Handle, bool bBroadcastReplaced, const FCombatMotionHandle& ReplacedHandle);
	void EndActiveMotion(FName EndReasonTag);

	void TickVelocityCurve(float DeltaTime);
	void TickRootMotion(float DeltaTime);
	void ApplyClampIfNeeded();
	void FinishTeleportRequestImmediately(const FCombatMotionRequest& Req, const FCombatMotionHandle& Handle);

	FVector MakeDirectionFromTarget(AActor* TargetActor) const;
	FVector ClampLocationToBattle(const FVector& InLocation, bool& bWasClamped) const;
};