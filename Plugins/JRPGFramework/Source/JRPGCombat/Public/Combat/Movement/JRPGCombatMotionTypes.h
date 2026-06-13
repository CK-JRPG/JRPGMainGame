#pragma once

#include "CoreMinimal.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "Curves/CurveFloat.h"
#include "Animation/AnimMontage.h"
#include "JRPGCombatMotionTypes.generated.h"

// -------------------------
// Motion Kind 
// -------------------------
UENUM()
enum class ECombatMotionType : uint8
{
	SkillMove,
	HitMove,
	GrappleMove
};

// -------------------------
// Archetype (확장/패턴 분리)
// -------------------------
UENUM()
enum class ECombatMotionArchetype : uint8
{
	None,
	Dash,
	Knockback,
	Knockup,
	KnockdownSlide,
	SlamToGround,
	WallSlam,
	PullToTarget,
	PushFromTarget
};

// -------------------------
// Exec Mode 
// -------------------------
UENUM()
enum class ECombatMotionExecMode : uint8
{
	DistanceReached,     // 수동: 거리 도달
	TimeElapsed,         // 수동: 시간 경과
	VelocityCurve,       // 수동: 커브 기반 속도

	RootMotion,          // 루트모션 몽타주
	Teleport,            // 순간 이동
	Launch               // LaunchCharacter 기반(넉업/슬램)
};

// -------------------------
// Block policy 
// -------------------------
UENUM()
enum class ECombatMotionEndPolicy : uint8
{
	StopOnBlock,
	SlideOnBlock,
	IgnoreBlock
};

// -------------------------
// Cancel policy 
// -------------------------
UENUM(meta=(Bitflags))
enum class ECombatMotionCancelPolicy : uint8
{
	None                = 0 UMETA(Hidden),
	OnNewHigherPriority = 1 << 0,
	OnHit               = 1 << 1,
	OnCC                = 1 << 2,
	Manual              = 1 << 3
};
ENUM_CLASS_FLAGS(ECombatMotionCancelPolicy);

// -------------------------
// Grapple sync mode (확장: 안전 동기화)
// -------------------------
UENUM()
enum class EGrappleSyncMode : uint8
{
	None,                 // 단순 이동(오너만)
	AttachOwnerToTarget,  // 오너를 타겟에 부착(그랩 동안)
	PullOwnerToTarget,    // 오너를 타겟 방향으로 당김(수동 이동)
	DragTargetToOwner,    // 타겟(캐릭터)도 오너 쪽으로 끌어옴(옵션)
	MeetAtMidpoint        // 오너/타겟이 둘 다 움직여 중간지점에서 만남(옵션)
};

// -------------------------
// Result (Public API)
// -------------------------
UENUM()
enum class ECombatMotionResult : uint8
{
	Accepted,
	Rejected,
	ReplacedExisting,
	Queued
};

// -------------------------
// Handle 
// -------------------------
USTRUCT()
struct FCombatMotionHandle
{
	GENERATED_BODY()

	UPROPERTY()
	FName OwnerTag = NAME_None;

	UPROPERTY()
	uint64 UniqueId = 0;

	bool IsValid() const { return UniqueId != 0; }
	void Invalidate() { OwnerTag = NAME_None; UniqueId = 0; }

	bool operator==(const FCombatMotionHandle& Other) const
	{
		return OwnerTag == Other.OwnerTag && UniqueId == Other.UniqueId;
	}
};

FORCEINLINE uint32 GetTypeHash(const FCombatMotionHandle& H)
{
	return HashCombine(GetTypeHash(H.OwnerTag), GetTypeHash(H.UniqueId));
}

// -------------------------
// Request 
// -------------------------
USTRUCT()
struct FCombatMotionRequest
{
	GENERATED_BODY()

	// 분류/패턴/실행
	UPROPERTY() ECombatMotionType Type = ECombatMotionType::SkillMove;
	UPROPERTY() ECombatMotionArchetype Archetype = ECombatMotionArchetype::None;
	UPROPERTY() ECombatMotionExecMode ExecMode = ECombatMotionExecMode::DistanceReached;

	// 우선순위(명시 시 강제)
	UPROPERTY() int32 Priority = -1;

	// 소유/디버그
	UPROPERTY() FName OwnerTag = NAME_None;   // CancelAllByOwner
	UPROPERTY() FName DebugTag = NAME_None;

	// 인과(피격 유발자/그랩 대상)
	UPROPERTY() TObjectPtr<AActor> Instigator = nullptr;
	UPROPERTY() TObjectPtr<AActor> Target = nullptr;

	// ---- 수동 이동 파라미터 ----
	UPROPERTY() FVector Direction = FVector::ZeroVector; // 월드 방향
	UPROPERTY() float Distance = 0.f;
	UPROPERTY() float Duration = 0.f;
	UPROPERTY() TObjectPtr<UCurveFloat> SpeedCurve = nullptr;

	// ---- RootMotion ----
	UPROPERTY() TObjectPtr<UAnimMontage> RootMontage = nullptr;

	// ---- Teleport ----
	UPROPERTY() FVector TeleportDest = FVector::ZeroVector;

	// ---- Launch (넉업/슬램/넉다운) ----
	UPROPERTY() FVector LaunchVelocity = FVector::ZeroVector;
	UPROPERTY() float LaunchMaxTime = 1.0f;          // 안전 타임아웃
	UPROPERTY() bool bEndLaunchWhenGrounded = true;  // 착지 시 종료

	// ---- Grapple sync ----
	UPROPERTY() EGrappleSyncMode GrappleSyncMode = EGrappleSyncMode::None;
	UPROPERTY() bool bAffectTargetIfCharacter = false; // Drag/Meet 모드에서 타겟도 움직일지
	UPROPERTY() float GrappleStopDistance = 80.f;      // PullOwnerToTarget 종료 거리
	UPROPERTY() FName GrappleAttachSocket = NAME_None; // AttachOwnerToTarget
	UPROPERTY() FVector GrappleAttachOffset = FVector::ZeroVector; // Attach offset in target space

	// ---- 충돌/클램프 ----
	UPROPERTY() ECombatMotionEndPolicy EndPolicy = ECombatMotionEndPolicy::StopOnBlock;
	UPROPERTY() bool bAllowZoneClamp = true;
	UPROPERTY() bool bRespectCollision = true; // IgnoreBlock이면 false로도 가능

	// ---- 회전/모드 ----
	UPROPERTY() bool bForceFlyingDuringManualMove = false;
	UPROPERTY() bool bFaceMoveDirection = false;
	UPROPERTY() float FaceYawInterpSpeed = 25.f;

	// ---- 마찰 ----
	UPROPERTY() bool bIgnoreFriction = false;

	// ---- 취소 ----
	UPROPERTY() bool bCancelable = true;
	UPROPERTY(meta=(Bitflags))
	int32 CancelPolicyBits =
		(int32)ECombatMotionCancelPolicy::OnNewHigherPriority |
		(int32)ECombatMotionCancelPolicy::OnHit |
		(int32)ECombatMotionCancelPolicy::Manual;

	ECombatMotionCancelPolicy GetCancelPolicy() const
	{
		return (ECombatMotionCancelPolicy)CancelPolicyBits;
	}
};

// -------------------------
// Response (Result + Handle + Reason)
// -------------------------
USTRUCT()
struct FCombatMotionResponse
{
	GENERATED_BODY()

	UPROPERTY() ECombatMotionResult Result = ECombatMotionResult::Rejected;
	UPROPERTY() FCombatMotionHandle Handle;
	UPROPERTY() FJRPGReason Reason;

	static FCombatMotionResponse Make(ECombatMotionResult R, const FCombatMotionHandle& H, const FJRPGReason& Reason)
	{
		FCombatMotionResponse X;
		X.Result = R; X.Handle = H; X.Reason = Reason;
		return X;
	}
};

// -------------------------
// Runtime State 
// -------------------------
USTRUCT()
struct FCombatMotionRuntimeState
{
	GENERATED_BODY()

	UPROPERTY() FCombatMotionHandle ActiveHandle;
	UPROPERTY() FCombatMotionRequest ActiveRequest;

	UPROPERTY() float StartRealTime = 0.f;
	UPROPERTY() float Elapsed = 0.f;
	UPROPERTY() float AccumDistance = 0.f;

	UPROPERTY() bool bBlocked = false;
	UPROPERTY() FHitResult LastBlockHit;

	// Grapple attach bookkeeping
	UPROPERTY() bool bOwnerAttachedToTarget = false;
	UPROPERTY() TWeakObjectPtr<AActor> AttachedTarget;
	UPROPERTY() TWeakObjectPtr<USceneComponent> PrevAttachParent;
	UPROPERTY() FName PrevAttachSocket = NAME_None;

	// Movement param restore
	UPROPERTY() TEnumAsByte<EMovementMode> PrevMovementMode = MOVE_Walking;
	UPROPERTY() float PrevGroundFriction = -1.f;
	UPROPERTY() float PrevBrakingFrictionFactor = -1.f;
	UPROPERTY() bool bHasSavedMovementParams = false;
};

// -------------------------
// Events
// -------------------------
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionStarted, const FCombatMotionHandle&, const FCombatMotionRequest&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionEnded, const FCombatMotionHandle&, FName /*EndReason*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionCancelled, const FCombatMotionHandle&, FName /*CancelReason*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionBlocked, const FCombatMotionHandle&, const FHitResult& /*Hit*/);

// 확장 이벤트(기획서 연동 포인트)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCombatMotionWallSlam, const FCombatMotionHandle&, const FHitResult& /*Hit*/, FName /*WallSlamTag*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionExternalHit, const FCombatMotionHandle&, FName /*HitTag*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionExternalCC, const FCombatMotionHandle&, bool /*bNowCC*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionGrappleAttach, const FCombatMotionHandle&, AActor* /*Target*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatMotionGrappleDetach, const FCombatMotionHandle&);