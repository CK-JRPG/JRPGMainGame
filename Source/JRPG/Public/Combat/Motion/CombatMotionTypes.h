#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Curves/CurveFloat.h"
#include "Animation/AnimMontage.h"
#include "CombatMotionTypes.generated.h"

UENUM()
enum class EJRPGCombatMotionType : uint8
{
	None,
	SkillMove,
	HitMove,
	GrappleMove
};


UENUM()
enum class EJRPGCombatMotionExecMode : uint8
{
	VelocityCurve,
	RootMotion,
	Teleport
};

UENUM()
enum class EJRPGCombatMotionEndPolicy : uint8
{
	TimeElapsed,
	DistanceReached,
	HitWallOrBlocked,
	MontageEnded,
	ExplicitCancel,
	TargetInvalid
};

UENUM()
enum class EJRPGCombatMotionResult : uint8
{
	Accepted,
	Rejected,
	ReplacedExisting,
	Queued
};

USTRUCT()
struct FJRPGCombatMotionHandle
{
	GENERATED_BODY()

	UPROPERTY()FName OwnerTag = NAME_None;
	UPROPERTY()uint64 UniqueId = 0;

	bool IsValid() const { return UniqueId != 0; }

	friend bool operator==(const FJRPGCombatMotionHandle& L, const FJRPGCombatMotionHandle& R)
	{
		return L.UniqueId == R.UniqueId && L.OwnerTag == R.OwnerTag;
	}
};

USTRUCT()
struct FJRPGCombatMotionRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere) EJRPGCombatMotionType Type = EJRPGCombatMotionType::SkillMove;
	UPROPERTY(EditAnywhere) EJRPGCombatMotionExecMode ExecMode = EJRPGCombatMotionExecMode::VelocityCurve;
	UPROPERTY(EditAnywhere) int32 Priority = 100;

	UPROPERTY() TWeakObjectPtr<AActor> Instigator;
	UPROPERTY() TWeakObjectPtr<AActor> Target;

	// 방향/목적지 해석
	UPROPERTY(EditAnywhere) FVector Direction = FVector::ZeroVector;
	UPROPERTY(EditAnywhere) bool bComputeDirectionFromTarget = false;
	UPROPERTY(EditAnywhere) float StopShortDistance = 0.f;

	UPROPERTY(EditAnywhere) float Distance = 0.f;
	UPROPERTY(EditAnywhere) float Duration = 0.20f;

	UPROPERTY(EditAnywhere) TObjectPtr<UCurveFloat> SpeedCurve = nullptr;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimMontage> RootMontage = nullptr;
	UPROPERTY(EditAnywhere) bool bMontageDrivenExternally = true;

	UPROPERTY(EditAnywhere) FVector TeleportDest = FVector::ZeroVector;
	UPROPERTY(EditAnywhere) bool bComputeTeleportFromTarget = false;

	UPROPERTY(EditAnywhere) bool bIgnoreFriction = false;
	UPROPERTY(EditAnywhere) EJRPGCombatMotionEndPolicy EndPolicy = EJRPGCombatMotionEndPolicy::TimeElapsed;

	UPROPERTY(EditAnywhere) bool bAllowClamp = true;
	UPROPERTY(EditAnywhere) bool bCancelable = true;

	UPROPERTY(EditAnywhere) bool bCancelOnHit = true;
	UPROPERTY(EditAnywhere) bool bCancelOnCC = true;
	UPROPERTY(EditAnywhere) bool bCancelOnNewHigherPriority = true;

	// 최소 게이트
	UPROPERTY(EditAnywhere) bool bAllowSkillMoveWhileGroggy = false;
	UPROPERTY(EditAnywhere) bool bAllowHitMoveWhileGroggy = true;

	UPROPERTY(EditAnywhere) FName OwnerTag = NAME_None;
	UPROPERTY(EditAnywhere) FName DebugTag = NAME_None;
};

USTRUCT()
struct FCombatMotionState
{
	GENERATED_BODY()

	UPROPERTY() FJRPGCombatMotionHandle ActiveHandle;
	UPROPERTY() FJRPGCombatMotionRequest ActiveRequest;

	UPROPERTY() FVector ResolvedDirection = FVector::ZeroVector;
	UPROPERTY() FVector ResolvedTeleportDest = FVector::ZeroVector;

	UPROPERTY() double StartTimeReal = 0.0;
	UPROPERTY() float Elapsed = 0.f;
	UPROPERTY() float AccumulatedDistance = 0.f;

	UPROPERTY() bool bBlocked = false;
	UPROPERTY() FHitResult LastBlockHitResult;
	UPROPERTY() bool bIsClampedThisFrame = false;

	UPROPERTY() FVector LastWorldLocation = FVector::ZeroVector;
};


USTRUCT()
struct FJRPGCombatMotionResponse
{
	GENERATED_BODY()
 
	UPROPERTY() EJRPGCombatMotionResult Result = EJRPGCombatMotionResult::Rejected;
	UPROPERTY() FJRPGCombatMotionHandle Handle;
	UPROPERTY() FName ReasonTag = NAME_None;
 
	static FJRPGCombatMotionResponse Make(EJRPGCombatMotionResult InResult, const FJRPGCombatMotionHandle& InHandle, FName InReason)
	{
		FJRPGCombatMotionResponse R;
		R.Result = InResult;
		R.Handle = InHandle;
		R.ReasonTag = InReason;
		return R;
	}
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionStarted, FJRPGCombatMotionHandle /*Handle*/, EJRPGCombatMotionType /*Type*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionEnded, FJRPGCombatMotionHandle /*Handle*/, FName /*EndReasonTag*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCombatMotionReplaced, FJRPGCombatMotionHandle /*OldHandle*/, FJRPGCombatMotionHandle /*NewHandle*/, FName /*ReasonTag*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatMotionBlocked, FJRPGCombatMotionHandle /*Handle*/, const FHitResult& /*Hit*/);
