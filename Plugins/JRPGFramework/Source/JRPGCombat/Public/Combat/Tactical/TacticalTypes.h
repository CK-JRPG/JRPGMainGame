#pragma once

#include "CoreMinimal.h"
#include "JRPGCoreApiTypes.h"
#include "TacticalTypes.generated.h"

UENUM()
enum class ETacticalState : uint8
{
	Idle,
	Entering,
	Active,
	Exiting
};

UENUM()
enum class ETacticalTargetKind : uint8
{
	None,
	Actor,
	Location
};

USTRUCT()
struct FTacticalTargetSnapshot
{
	GENERATED_BODY()

	UPROPERTY() ETacticalTargetKind Kind = ETacticalTargetKind::None;
	UPROPERTY() TWeakObjectPtr<AActor> TargetActor;
	UPROPERTY() FVector TargetLocation = FVector::ZeroVector;

	static FTacticalTargetSnapshot MakeNone()
	{
		FTacticalTargetSnapshot S;
		S.Kind = ETacticalTargetKind::None;
		return S;
	}

	static FTacticalTargetSnapshot MakeActor(AActor* A)
	{
		FTacticalTargetSnapshot S;
		S.Kind = ETacticalTargetKind::Actor;
		S.TargetActor = A;
		return S;
	}

	static FTacticalTargetSnapshot MakeLocation(const FVector& L)
	{
		FTacticalTargetSnapshot S;
		S.Kind = ETacticalTargetKind::Location;
		S.TargetLocation = L;
		return S;
	}

	bool HasActor() const { return Kind == ETacticalTargetKind::Actor && TargetActor.IsValid(); }
	bool HasLocation() const { return Kind == ETacticalTargetKind::Location; }
};

UENUM(meta=(Bitflags))
enum class ETacticalReservationFlags : uint8
{
	None   = 0 UMETA(Hidden),
	Queued = 1 << 0,
	Ready  = 1 << 1
};
ENUM_CLASS_FLAGS(ETacticalReservationFlags);

USTRUCT()
struct FTacticalReservation
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	UPROPERTY() FName SkillId = NAME_None;
	UPROPERTY() FTacticalTargetSnapshot Target;
	UPROPERTY() double CreatedAtReal = 0.0;

	UPROPERTY(meta=(Bitflags))
	uint8 FlagsBits = (uint8)ETacticalReservationFlags::None;

	ETacticalReservationFlags GetFlags() const { return (ETacticalReservationFlags)FlagsBits; }
	void SetFlags(ETacticalReservationFlags F) { FlagsBits = (uint8)F; }
};

/**
 * UI 표시용 스냅샷(폴링 API 반환값)
 * - WeakPtr는 UI에서 안전하게 사용(대상이 삭제되면 nullptr로 떨어짐)
 */
USTRUCT()
struct FTacticalReservationView
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	UPROPERTY() FName SkillId = NAME_None;
	UPROPERTY() ETacticalReservationFlags Flags = ETacticalReservationFlags::None;

	UPROPERTY() ETacticalTargetKind TargetKind = ETacticalTargetKind::None;
	UPROPERTY() TWeakObjectPtr<AActor> TargetActor;
	UPROPERTY() FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY() double CreatedAtReal = 0.0;

	bool IsReady() const { return EnumHasAnyFlags(Flags, ETacticalReservationFlags::Ready); }
	bool IsQueued() const { return EnumHasAnyFlags(Flags, ETacticalReservationFlags::Queued); }
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTacticalStateChanged, ETacticalState /*Prev*/, ETacticalState /*New*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTacticalReservationChanged, AActor* /*Actor*/, bool /*HasReservation*/, FName /*SkillId*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTacticalReservationFlagsChanged, AActor* /*Actor*/, FName /*SkillId*/, ETacticalReservationFlags /*Flags*/);

/**
 * UI 타이머 갱신 이벤트(옵션)
 * - 폴링 대신 이벤트 기반 UI 갱신에 사용 가능
 */
DECLARE_MULTICAST_DELEGATE_FourParams(
	FOnTacticalTimerUpdated,
	float /*ElapsedRealSec*/,
	float /*RemainingRealSec*/,
	float /*Normalized01*/,
	bool  /*bActive*/
);