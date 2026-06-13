#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "JRPGCombatMotionTypes.h"
#include "JRPGCombatMotionComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class ULocomotionComponent;
class ACombatZoneActor;

UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UJRPGCombatMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJRPGCombatMotionComponent();

	// -------- Public API --------
	FCombatMotionResponse RequestCombatMotion(const FCombatMotionRequest& Req);
	FJRPGOpResult CancelCombatMotion(const FCombatMotionHandle& Handle, FName ReasonTag);
	FJRPGOpResult CancelAllByOwner(FName OwnerTag);

	// CancelPolicy 자동 트리거(기획서 연동용)
	// - 피해 처리/상태이상/그로기/체인 억제 등 외부 시스템이 호출
	void NotifyExternalHit(FName HitTag);              // CancelPolicy::OnHit
	void NotifyExternalCCStateChanged(bool bNowCC);    // CancelPolicy::OnCC

	bool IsMotionActive() const { return State.ActiveHandle.IsValid(); }
	const FCombatMotionRuntimeState& GetRuntimeState() const { return State; }

	// -------- Events --------
	FOnCombatMotionStarted   OnMotionStarted;
	FOnCombatMotionEnded     OnMotionEnded;
	FOnCombatMotionCancelled OnMotionCancelled;
	FOnCombatMotionBlocked   OnMotionBlocked;

	// 확장 이벤트(벽꿍/외부 트리거/그랩)
	FOnCombatMotionWallSlam      OnWallSlam;
	FOnCombatMotionExternalHit   OnExternalHit;
	FOnCombatMotionExternalCC    OnExternalCC;
	FOnCombatMotionGrappleAttach OnGrappleAttach;
	FOnCombatMotionGrappleDetach OnGrappleDetach;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient) TObjectPtr<ACharacter> OwnerCharacter = nullptr;
	UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> CharMove = nullptr;
	UPROPERTY(Transient) TObjectPtr<ULocomotionComponent> Locomotion = nullptr;

	UPROPERTY(Transient) FCombatMotionRuntimeState State;
	UPROPERTY(Transient) TArray<FCombatMotionRequest> PendingQueue;

	FJRPGHandle LocomotionLockHandle;
	uint64 NextUniqueId = 1;
	float LastHitMoveRequestRealSec = -1000.f;

	// default priorities
	UPROPERTY(EditAnywhere, Category="JRPG|CombatMotion|Priority") int32 Priority_Skill = 10;
	UPROPERTY(EditAnywhere, Category="JRPG|CombatMotion|Priority") int32 Priority_Hit = 20;
	UPROPERTY(EditAnywhere, Category="JRPG|CombatMotion|Priority") int32 Priority_Grapple = 30;

	// safety
	UPROPERTY(EditAnywhere, Category="JRPG|CombatMotion|Safety") float ManualMoveDefaultSpeed = 2000.f;
	UPROPERTY(EditAnywhere, Category="JRPG|CombatMotion|Safety") float ZoneClampSearchMaxActors = 8; // 오버랩 탐색 상한(보조 안전)

	// Internals
	void CacheOwnerRefs();

	int32 ResolvePriority(const FCombatMotionRequest& Req) const;
	bool ValidateRequest(const FCombatMotionRequest& Req, FJRPGReason& OutReason) const;

	// CC/Groggy query (임시: Tag 기반)
	bool IsCCActive() const;
	bool IsGroggyStunned() const;
	bool CanRequestDuringCC(const FCombatMotionRequest& Req) const;

	// Replacement / queue
	bool ShouldReplaceActive(const FCombatMotionRequest& NewReq, int32 NewPriority) const;
	bool ShouldQueueInsteadOfReject(const FCombatMotionRequest& Req) const;

	// lifecycle
	FCombatMotionHandle MakeHandle(const FCombatMotionRequest& Req);
	void StartMotion_Internal(const FCombatMotionRequest& Req, const FCombatMotionHandle& Handle);
	void EndMotion_Internal(FName EndReasonTag);
	void CancelMotion_Internal(FName CancelReasonTag);

	// movement param save/restore
	void SaveMovementParamsIfNeeded();
	void RestoreMovementParamsIfNeeded();

	// grapple attach helpers
	bool TryAttachOwnerToTarget(AActor* Target, const FName Socket, const FVector& OffsetInTargetSpace);
	void DetachOwnerFromTargetIfNeeded();

	// locomotion lock
	void EnsureLocomotionLocked();
	void EnsureLocomotionUnlocked();

	// tick
	void TickActiveMotion(float DeltaTime);
	void Tick_DistanceReached(float DeltaTime);
	void Tick_TimeElapsed(float DeltaTime);
	void Tick_VelocityCurve(float DeltaTime);
	void Tick_RootMotion(float DeltaTime);
	void Tick_Launch(float DeltaTime);
	void Exec_Teleport();

	// grapple tick helpers
	void Tick_GrappleSync(float DeltaTime);
	void ApplyDragTargetToOwner(float DeltaTime);
	void ApplyMeetAtMidpoint(float DeltaTime);

	// movement apply
	void FaceDirectionIfNeeded(const FVector& MoveDir, float DeltaTime);
	bool SafeMoveStep(const FVector& Delta, bool bRespectCollision, FHitResult& OutHit);

	// clamp safety
	void ApplyZoneClampIfAvailable();
	bool IsGrounded() const;

	// queue
	bool TryDequeueAndStart();
};
