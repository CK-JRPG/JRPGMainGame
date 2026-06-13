#include "Combat/Movement/JRPGCombatMotionComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"

#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Session/CombatZoneActor.h"

UJRPGCombatMotionComponent::UJRPGCombatMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UJRPGCombatMotionComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerRefs();
}

void UJRPGCombatMotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PendingQueue.Reset();
	DetachOwnerFromTargetIfNeeded();
	EnsureLocomotionUnlocked();
	RestoreMovementParamsIfNeeded();
	State = FCombatMotionRuntimeState();
	Super::EndPlay(EndPlayReason);
}

void UJRPGCombatMotionComponent::CacheOwnerRefs()
{
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatMotion] Owner must be ACharacter."));
		return;
	}

	CharMove = OwnerCharacter->GetCharacterMovement();
	if (!CharMove)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatMotion] Missing CharacterMovementComponent."));
	}

	Locomotion = OwnerCharacter->FindComponentByClass<ULocomotionComponent>();
}

int32 UJRPGCombatMotionComponent::ResolvePriority(const FCombatMotionRequest& Req) const
{
	if (Req.Priority >= 0) return Req.Priority;

	switch (Req.Type)
	{
	case ECombatMotionType::SkillMove:   return Priority_Skill;
	case ECombatMotionType::HitMove:     return Priority_Hit;
	case ECombatMotionType::GrappleMove: return Priority_Grapple;
	default: return Priority_Skill;
	}
}

bool UJRPGCombatMotionComponent::IsCCActive() const
{
	// 완전 구현(임시): ActorTag 기반
	return OwnerCharacter && OwnerCharacter->ActorHasTag(TEXT("CC"));
}

bool UJRPGCombatMotionComponent::IsGroggyStunned() const
{
	return OwnerCharacter && OwnerCharacter->ActorHasTag(TEXT("Groggy_Stunned"));
}

bool UJRPGCombatMotionComponent::CanRequestDuringCC(const FCombatMotionRequest& Req) const
{
	if (!IsCCActive()) return true;
	// 기획서: CC 중 SkillMove는 거부, Hit/Grapple은 허용
	return (Req.Type != ECombatMotionType::SkillMove);
}

bool UJRPGCombatMotionComponent::ValidateRequest(const FCombatMotionRequest& Req, FJRPGReason& OutReason) const
{
	if (!OwnerCharacter || !CharMove)
	{
		OutReason = FJRPGReason::Make("Reject.NoOwner");
		return false;
	}

	// Groggy: Stun 중 SkillMove 거부
	if (IsGroggyStunned() && Req.Type == ECombatMotionType::SkillMove)
	{
		OutReason = FJRPGReason::Make("Reject.GroggyStunned");
		return false;
	}

	// CC 정책
	if (!CanRequestDuringCC(Req))
	{
		OutReason = FJRPGReason::Make("Reject.CCBlocked");
		return false;
	}

	// ExecMode별 검증
	switch (Req.ExecMode)
	{
	case ECombatMotionExecMode::Teleport:
		return true;

	case ECombatMotionExecMode::RootMotion:
		if (!Req.RootMontage)
		{
			OutReason = FJRPGReason::Make("Reject.NoRootMontage");
			return false;
		}
		return true;

	case ECombatMotionExecMode::Launch:
		if (Req.LaunchMaxTime <= 0.f)
		{
			OutReason = FJRPGReason::Make("Reject.BadLaunchTime");
			return false;
		}
		return true;

	case ECombatMotionExecMode::VelocityCurve:
		if (!Req.SpeedCurve)
		{
			OutReason = FJRPGReason::Make("Reject.NoSpeedCurve");
			return false;
		}
		// fallthrough
	case ECombatMotionExecMode::DistanceReached:
		if (Req.Distance <= 0.f || Req.Direction.IsNearlyZero())
		{
			OutReason = FJRPGReason::Make("Reject.BadDistanceOrDirection");
			return false;
		}
		return true;

	case ECombatMotionExecMode::TimeElapsed:
		if (Req.Duration <= 0.f || Req.Distance <= 0.f || Req.Direction.IsNearlyZero())
		{
			OutReason = FJRPGReason::Make("Reject.BadTimeOrDistanceOrDirection");
			return false;
		}
		return true;

	default:
		OutReason = FJRPGReason::Make("Reject.BadExecMode");
		return false;
	}
}

bool UJRPGCombatMotionComponent::ShouldReplaceActive(const FCombatMotionRequest& NewReq, int32 NewPriority) const
{
	if (!State.ActiveHandle.IsValid()) return true;

	// 강제 규칙:
	// Grapple > Hit > Skill
	if (NewReq.Type == ECombatMotionType::GrappleMove && State.ActiveRequest.Type != ECombatMotionType::GrappleMove)
		return true;
	if (NewReq.Type == ECombatMotionType::HitMove && State.ActiveRequest.Type == ECombatMotionType::SkillMove)
		return true;

	const int32 ActivePriority = ResolvePriority(State.ActiveRequest);
	if (NewPriority > ActivePriority) return true;

	// 동률이면 Hit/Grapple만 교체 허용(스킬 이동은 연출 중복 방지)
	if (NewPriority == ActivePriority &&
		NewReq.Type == State.ActiveRequest.Type &&
		(NewReq.Type == ECombatMotionType::HitMove || NewReq.Type == ECombatMotionType::GrappleMove))
	{
		return true;
	}

	return false;
}

bool UJRPGCombatMotionComponent::ShouldQueueInsteadOfReject(const FCombatMotionRequest& Req) const
{
	// SkillMove만 큐잉(기획서 Queued 결과)
	return Req.Type == ECombatMotionType::SkillMove;
}

FCombatMotionHandle UJRPGCombatMotionComponent::MakeHandle(const FCombatMotionRequest& Req)
{
	FCombatMotionHandle H;
	H.OwnerTag = Req.OwnerTag.IsNone()
		? (Req.Type == ECombatMotionType::SkillMove ? FName("Skill")
			: Req.Type == ECombatMotionType::HitMove ? FName("Hit")
			: FName("Grapple"))
		: Req.OwnerTag;

	H.UniqueId = NextUniqueId++;
	return H;
}

// ----------------------------------
// Public API
// ----------------------------------
FCombatMotionResponse UJRPGCombatMotionComponent::RequestCombatMotion(const FCombatMotionRequest& Req)
{
	FJRPGReason Reason;
	if (!ValidateRequest(Req, Reason))
	{
		return FCombatMotionResponse::Make(ECombatMotionResult::Rejected, {}, Reason);
	}

	const int32 NewPriority = ResolvePriority(Req);
	if (Req.Type == ECombatMotionType::HitMove)
	{
		const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
		if (State.ActiveHandle.IsValid() && State.ActiveRequest.Type == ECombatMotionType::HitMove)
		{
			return FCombatMotionResponse::Make(ECombatMotionResult::Rejected, State.ActiveHandle, FJRPGReason::Make("Reject.HitReactAlreadyActive"));
		}
		if (Now - LastHitMoveRequestRealSec < 0.35f)
		{
			return FCombatMotionResponse::Make(ECombatMotionResult::Rejected, {}, FJRPGReason::Make("Reject.HitReactCooldown"));
		}
	}

	if (State.ActiveHandle.IsValid())
	{
		if (ShouldReplaceActive(Req, NewPriority))
		{
			// 기존 모션 취소(시스템 안정성 우선)
			CancelMotion_Internal(FName("Cancel.Replaced"));

			const FCombatMotionHandle H = MakeHandle(Req);
			StartMotion_Internal(Req, H);
			if (Req.Type == ECombatMotionType::HitMove)
			{
				LastHitMoveRequestRealSec = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
			}
			return FCombatMotionResponse::Make(ECombatMotionResult::ReplacedExisting, H, FJRPGReason::None());
		}

		if (ShouldQueueInsteadOfReject(Req))
		{
			PendingQueue.Add(Req);
			return FCombatMotionResponse::Make(ECombatMotionResult::Queued, {}, FJRPGReason::Make("Queued.LowerPriority"));
		}

		return FCombatMotionResponse::Make(ECombatMotionResult::Rejected, {}, FJRPGReason::Make("Reject.LowerPriority"));
	}

	const FCombatMotionHandle H = MakeHandle(Req);
	StartMotion_Internal(Req, H);
	if (Req.Type == ECombatMotionType::HitMove)
	{
		LastHitMoveRequestRealSec = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
	}
	return FCombatMotionResponse::Make(ECombatMotionResult::Accepted, H, FJRPGReason::None());
}

FJRPGOpResult UJRPGCombatMotionComponent::CancelCombatMotion(const FCombatMotionHandle& Handle, FName ReasonTag)
{
	if (!Handle.IsValid())
	{
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Cancel.InvalidHandle"));
	}

	if (State.ActiveHandle.IsValid() && State.ActiveHandle == Handle)
	{
		if (!State.ActiveRequest.bCancelable ||
			!EnumHasAnyFlags(State.ActiveRequest.GetCancelPolicy(), ECombatMotionCancelPolicy::Manual))
		{
			return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("Cancel.NotAllowed"));
		}

		CancelMotion_Internal(ReasonTag.IsNone() ? FName("Cancel.Manual") : ReasonTag);
		return FJRPGOpResult::Ok();
	}

	// 큐 제거(큐는 handle이 없으므로 OwnerTag 기준)
	const int32 Before = PendingQueue.Num();
	PendingQueue.RemoveAll([&](const FCombatMotionRequest& R) { return R.OwnerTag == Handle.OwnerTag; });

	if (PendingQueue.Num() != Before)
		return FJRPGOpResult::Ok();

	return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Cancel.NotFound"));
}

FJRPGOpResult UJRPGCombatMotionComponent::CancelAllByOwner(FName OwnerTag)
{
	if (OwnerTag.IsNone())
	{
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("CancelAll.InvalidOwner"));
	}

	if (State.ActiveHandle.IsValid() && State.ActiveHandle.OwnerTag == OwnerTag)
	{
		CancelMotion_Internal(FName("Cancel.Owner"));
	}

	PendingQueue.RemoveAll([&](const FCombatMotionRequest& R) { return R.OwnerTag == OwnerTag; });
	return FJRPGOpResult::Ok();
}

// ----------------------------------
// CancelPolicy Auto Triggers
// ----------------------------------
void UJRPGCombatMotionComponent::NotifyExternalHit(FName HitTag)
{
	if (!State.ActiveHandle.IsValid()) return;

	OnExternalHit.Broadcast(State.ActiveHandle, HitTag);

	// CancelPolicy::OnHit
	if (EnumHasAnyFlags(State.ActiveRequest.GetCancelPolicy(), ECombatMotionCancelPolicy::OnHit))
	{
		// 보통: SkillMove/그랩은 맞으면 끊기게
		CancelMotion_Internal(FName("Cancel.OnHit"));
	}
}

void UJRPGCombatMotionComponent::NotifyExternalCCStateChanged(bool bNowCC)
{
	if (!State.ActiveHandle.IsValid()) return;

	OnExternalCC.Broadcast(State.ActiveHandle, bNowCC);

	// CC가 켜졌을 때만 취소
	if (bNowCC && EnumHasAnyFlags(State.ActiveRequest.GetCancelPolicy(), ECombatMotionCancelPolicy::OnCC))
	{
		CancelMotion_Internal(FName("Cancel.OnCC"));
	}
}

// ----------------------------------
// Movement param save/restore
// ----------------------------------
void UJRPGCombatMotionComponent::SaveMovementParamsIfNeeded()
{
	if (!CharMove || State.bHasSavedMovementParams) return;

	State.PrevMovementMode = CharMove->MovementMode;
	State.PrevGroundFriction = CharMove->GroundFriction;
	State.PrevBrakingFrictionFactor = CharMove->BrakingFrictionFactor;
	State.bHasSavedMovementParams = true;
}

void UJRPGCombatMotionComponent::RestoreMovementParamsIfNeeded()
{
	if (!CharMove || !State.bHasSavedMovementParams) return;

	CharMove->SetMovementMode(State.PrevMovementMode);
	CharMove->GroundFriction = State.PrevGroundFriction;
	CharMove->BrakingFrictionFactor = State.PrevBrakingFrictionFactor;

	State.bHasSavedMovementParams = false;
}

// ----------------------------------
// Locomotion Lock
// ----------------------------------
void UJRPGCombatMotionComponent::EnsureLocomotionLocked()
{
	if (!Locomotion) return;
	if (LocomotionLockHandle.IsValid()) return;

	const auto Res = Locomotion->AcquireInputLock(FName("Locomotion.CombatMotion"));
	if (Res.bOk)
	{
		LocomotionLockHandle = Res.Value;
	}
}

void UJRPGCombatMotionComponent::EnsureLocomotionUnlocked()
{
	if (!Locomotion) return;
	if (!LocomotionLockHandle.IsValid()) return;

	Locomotion->ReleaseInputLock(LocomotionLockHandle);
	LocomotionLockHandle = FJRPGHandle();
}

// ----------------------------------
// Grapple Attach
// ----------------------------------
bool UJRPGCombatMotionComponent::TryAttachOwnerToTarget(AActor* Target, const FName Socket, const FVector& OffsetInTargetSpace)
{
	if (!OwnerCharacter || !Target) return false;

	USceneComponent* TargetRoot = Target->GetRootComponent();
	if (!TargetRoot) return false;

	// 저장(원복)
	State.PrevAttachParent = OwnerCharacter->GetRootComponent()->GetAttachParent();
	State.PrevAttachSocket = OwnerCharacter->GetAttachParentSocketName();

	// Attach
	FAttachmentTransformRules Rules(EAttachmentRule::KeepWorld, true);
	OwnerCharacter->AttachToComponent(TargetRoot, Rules, Socket);

	// offset 적용(타겟 로컬 오프셋을 월드로)
	if (!OffsetInTargetSpace.IsNearlyZero())
	{
		const FTransform TR = TargetRoot->GetComponentTransform();
		const FVector WorldOffset = TR.TransformVectorNoScale(OffsetInTargetSpace);
		OwnerCharacter->SetActorLocation(OwnerCharacter->GetActorLocation() + WorldOffset, false, nullptr, ETeleportType::TeleportPhysics);
	}

	State.bOwnerAttachedToTarget = true;
	State.AttachedTarget = Target;
	OnGrappleAttach.Broadcast(State.ActiveHandle, Target);
	return true;
}

void UJRPGCombatMotionComponent::DetachOwnerFromTargetIfNeeded()
{
	if (!OwnerCharacter) return;
	if (!State.bOwnerAttachedToTarget) return;

	OwnerCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	State.bOwnerAttachedToTarget = false;
	State.AttachedTarget.Reset();
	OnGrappleDetach.Broadcast(State.ActiveHandle);
}

// ----------------------------------
// Lifecycle
// ----------------------------------
void UJRPGCombatMotionComponent::StartMotion_Internal(const FCombatMotionRequest& Req, const FCombatMotionHandle& Handle)
{
	State = FCombatMotionRuntimeState();
	State.ActiveHandle = Handle;
	State.ActiveRequest = Req;
	State.StartRealTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;

	SaveMovementParamsIfNeeded();
	EnsureLocomotionLocked();

	// friction override
	if (CharMove && Req.bIgnoreFriction)
	{
		CharMove->GroundFriction = 0.f;
		CharMove->BrakingFrictionFactor = 0.f;
	}

	// manual move vertical requirement
	if (CharMove && Req.bForceFlyingDuringManualMove)
	{
		CharMove->SetMovementMode(MOVE_Flying);
	}

	// Grapple: Attach mode
	if (Req.Type == ECombatMotionType::GrappleMove && Req.GrappleSyncMode == EGrappleSyncMode::AttachOwnerToTarget)
	{
		if (Req.Target)
		{
			TryAttachOwnerToTarget(Req.Target, Req.GrappleAttachSocket, Req.GrappleAttachOffset);
		}
		// Attach만 하는 경우 Duration 기반으로 끝나야 안정적(없으면 기본 0.25)
		if (State.ActiveRequest.ExecMode == ECombatMotionExecMode::DistanceReached)
		{
			// attach 동안은 거리 이동이 의미 없을 수 있으니 안전하게 TimeElapsed로 전환
			State.ActiveRequest.ExecMode = ECombatMotionExecMode::TimeElapsed;
			State.ActiveRequest.Duration = FMath::Max(0.25f, State.ActiveRequest.Duration);
			State.ActiveRequest.Distance = FMath::Max(1.f, State.ActiveRequest.Distance);
			State.ActiveRequest.Direction = FVector::ForwardVector;
		}
	}

	// RootMotion
	if (Req.ExecMode == ECombatMotionExecMode::RootMotion && Req.RootMontage && OwnerCharacter)
	{
		OwnerCharacter->PlayAnimMontage(Req.RootMontage);
	}

	// Teleport
	if (Req.ExecMode == ECombatMotionExecMode::Teleport)
	{
		OnMotionStarted.Broadcast(Handle, Req);
		Exec_Teleport();
		EndMotion_Internal(FName("End.Teleport"));
		return;
	}

	// Launch
	if (Req.ExecMode == ECombatMotionExecMode::Launch && OwnerCharacter)
	{
		OwnerCharacter->LaunchCharacter(Req.LaunchVelocity, true, true);
	}

	OnMotionStarted.Broadcast(Handle, Req);
}

void UJRPGCombatMotionComponent::EndMotion_Internal(FName EndReasonTag)
{
	const FCombatMotionHandle Ended = State.ActiveHandle;

	DetachOwnerFromTargetIfNeeded();
	RestoreMovementParamsIfNeeded();
	State = FCombatMotionRuntimeState();

	EnsureLocomotionUnlocked();
	OnMotionEnded.Broadcast(Ended, EndReasonTag);

	TryDequeueAndStart();
}

void UJRPGCombatMotionComponent::CancelMotion_Internal(FName CancelReasonTag)
{
	const FCombatMotionHandle Cancelled = State.ActiveHandle;

	// RootMotion cancel
	if (State.ActiveRequest.ExecMode == ECombatMotionExecMode::RootMotion && State.ActiveRequest.RootMontage && OwnerCharacter)
	{
		OwnerCharacter->StopAnimMontage(State.ActiveRequest.RootMontage);
	}

	DetachOwnerFromTargetIfNeeded();
	RestoreMovementParamsIfNeeded();
	State = FCombatMotionRuntimeState();

	EnsureLocomotionUnlocked();
	OnMotionCancelled.Broadcast(Cancelled, CancelReasonTag);

	TryDequeueAndStart();
}

// ----------------------------------
// Tick
// ----------------------------------
void UJRPGCombatMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || !CharMove) return;

	if (!State.ActiveHandle.IsValid())
	{
		TryDequeueAndStart();
		return;
	}

	TickActiveMotion(DeltaTime);

	if (State.ActiveRequest.bAllowZoneClamp)
	{
		ApplyZoneClampIfAvailable();
	}
}

void UJRPGCombatMotionComponent::TickActiveMotion(float DeltaTime)
{
	State.Elapsed += DeltaTime;

	// Grapple sync (attach/pull/drag) is orthogonal; execute before/after move step as needed.
	Tick_GrappleSync(DeltaTime);

	switch (State.ActiveRequest.ExecMode)
	{
	case ECombatMotionExecMode::DistanceReached: Tick_DistanceReached(DeltaTime); break;
	case ECombatMotionExecMode::TimeElapsed:     Tick_TimeElapsed(DeltaTime); break;
	case ECombatMotionExecMode::VelocityCurve:   Tick_VelocityCurve(DeltaTime); break;
	case ECombatMotionExecMode::RootMotion:      Tick_RootMotion(DeltaTime); break;
	case ECombatMotionExecMode::Launch:          Tick_Launch(DeltaTime); break;
	default: break;
	}
}

void UJRPGCombatMotionComponent::Tick_GrappleSync(float DeltaTime)
{
	const FCombatMotionRequest& Req = State.ActiveRequest;
	if (Req.Type != ECombatMotionType::GrappleMove) return;

	switch (Req.GrappleSyncMode)
	{
	case EGrappleSyncMode::DragTargetToOwner:
		ApplyDragTargetToOwner(DeltaTime);
		break;
	case EGrappleSyncMode::MeetAtMidpoint:
		ApplyMeetAtMidpoint(DeltaTime);
		break;
	default:
		break;
	}
}

void UJRPGCombatMotionComponent::ApplyDragTargetToOwner(float DeltaTime)
{
	const FCombatMotionRequest& Req = State.ActiveRequest;
	if (!Req.bAffectTargetIfCharacter) return;
	if (!Req.Target) return;

	ACharacter* TargetChar = Cast<ACharacter>(Req.Target.Get());
	if (!TargetChar) return;

	UCharacterMovementComponent* TargetMove = TargetChar->GetCharacterMovement();
	if (!TargetMove) return;

	const FVector OwnerPos = OwnerCharacter->GetActorLocation();
	const FVector TargetPos = TargetChar->GetActorLocation();

	const FVector ToOwner = (OwnerPos - TargetPos);
	const float Dist = ToOwner.Size2D();
	if (Dist <= Req.GrappleStopDistance)
	{
		// 충분히 가까우면 종료(그랩 연출 완료)
		EndMotion_Internal(FName("End.GrappleCloseEnough"));
		return;
	}

	const FVector Dir = FVector(ToOwner.X, ToOwner.Y, 0.f).GetSafeNormal();
	const float PullSpeed = FMath::Max(600.f, ManualMoveDefaultSpeed * 0.6f);
	const float Step = PullSpeed * DeltaTime;

	FHitResult Hit;
	TargetMove->SafeMoveUpdatedComponent(Dir * Step, TargetChar->GetActorRotation(), true, Hit);

	// Target가 벽에 막히면 그냥 유지(추가 처리 가능)
}

void UJRPGCombatMotionComponent::ApplyMeetAtMidpoint(float DeltaTime)
{
	const FCombatMotionRequest& Req = State.ActiveRequest;
	if (!Req.bAffectTargetIfCharacter) return;
	if (!Req.Target) return;

	ACharacter* TargetChar = Cast<ACharacter>(Req.Target.Get());
	if (!TargetChar) return;

	UCharacterMovementComponent* TargetMove = TargetChar->GetCharacterMovement();
	if (!TargetMove) return;

	const FVector OwnerPos = OwnerCharacter->GetActorLocation();
	const FVector TargetPos = TargetChar->GetActorLocation();
	const FVector Mid = (OwnerPos + TargetPos) * 0.5f;

	const FVector OwnerToMid = (Mid - OwnerPos);
	const FVector TargetToMid = (Mid - TargetPos);

	const float DistOwner = OwnerToMid.Size2D();
	const float DistTarget = TargetToMid.Size2D();

	if (DistOwner <= State.ActiveRequest.GrappleStopDistance && DistTarget <= State.ActiveRequest.GrappleStopDistance)
	{
		EndMotion_Internal(FName("End.GrappleMeet"));
		return;
	}

	const float Speed = FMath::Max(800.f, ManualMoveDefaultSpeed * 0.7f);
	const float Step = Speed * DeltaTime;

	// Owner step (xy only)
	{
		const FVector Dir = FVector(OwnerToMid.X, OwnerToMid.Y, 0.f).GetSafeNormal();
		FHitResult Hit;
		SafeMoveStep(Dir * Step, true, Hit);
	}

	// Target step
	{
		const FVector Dir = FVector(TargetToMid.X, TargetToMid.Y, 0.f).GetSafeNormal();
		FHitResult Hit;
		TargetMove->SafeMoveUpdatedComponent(Dir * Step, TargetChar->GetActorRotation(), true, Hit);
	}
}

void UJRPGCombatMotionComponent::FaceDirectionIfNeeded(const FVector& MoveDir, float DeltaTime)
{
	if (!OwnerCharacter) return;
	if (!State.ActiveRequest.bFaceMoveDirection) return;

	const FVector Flat(MoveDir.X, MoveDir.Y, 0.f);
	if (Flat.IsNearlyZero()) return;

	const FRotator TargetRot = Flat.Rotation();
	const FRotator Current = OwnerCharacter->GetActorRotation();

	const float Speed = FMath::Max(1.f, State.ActiveRequest.FaceYawInterpSpeed);
	const FRotator NewRot = FMath::RInterpTo(Current, TargetRot, DeltaTime, Speed);
	OwnerCharacter->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}

bool UJRPGCombatMotionComponent::SafeMoveStep(const FVector& Delta, bool bRespectCollision, FHitResult& OutHit)
{
	if (!CharMove) return false;
	OutHit = FHitResult();
	CharMove->SafeMoveUpdatedComponent(Delta, OwnerCharacter->GetActorRotation(), bRespectCollision, OutHit);
	return !OutHit.IsValidBlockingHit();
}

void UJRPGCombatMotionComponent::Tick_DistanceReached(float DeltaTime)
{
	FCombatMotionRequest& Req = State.ActiveRequest;

	// Grapple PullOwnerToTarget: Direction을 매 프레임 갱신해도 됨(확장성)
	if (Req.Type == ECombatMotionType::GrappleMove && Req.GrappleSyncMode == EGrappleSyncMode::PullOwnerToTarget && Req.Target)
	{
		const FVector OwnerPos = OwnerCharacter->GetActorLocation();
		const FVector TargetPos = Req.Target->GetActorLocation();
		const FVector ToTarget = TargetPos - OwnerPos;

		if (ToTarget.Size2D() <= Req.GrappleStopDistance)
		{
			EndMotion_Internal(FName("End.GrappleCloseEnough"));
			return;
		}

		Req.Direction = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
	}

	const FVector Dir = Req.Direction.GetSafeNormal();
	const float Remaining = FMath::Max(0.f, Req.Distance - State.AccumDistance);
	if (Remaining <= KINDA_SMALL_NUMBER)
	{
		EndMotion_Internal(FName("End.DistanceReached"));
		return;
	}

	const float Speed = (Req.Duration > 0.f) ? (Req.Distance / Req.Duration) : ManualMoveDefaultSpeed;
	const float Step = FMath::Min(Remaining, Speed * DeltaTime);

	FaceDirectionIfNeeded(Dir, DeltaTime);

	FHitResult Hit;
	SafeMoveStep(Dir * Step, Req.EndPolicy != ECombatMotionEndPolicy::IgnoreBlock, Hit);
	State.AccumDistance += Step;

	if (Hit.IsValidBlockingHit())
	{
		State.bBlocked = true;
		State.LastBlockHit = Hit;
		OnMotionBlocked.Broadcast(State.ActiveHandle, Hit);

		// 벽꿍: 블록 순간에 확장 이벤트 발행(상태이상/그로기/SP/어그로는 구독으로 연동)
		if (Req.Archetype == ECombatMotionArchetype::WallSlam)
		{
			OnWallSlam.Broadcast(State.ActiveHandle, Hit, FName("WallSlam"));
		}

		if (Req.EndPolicy == ECombatMotionEndPolicy::StopOnBlock)
		{
			EndMotion_Internal(Req.Archetype == ECombatMotionArchetype::WallSlam ? FName("End.WallSlam") : FName("End.Blocked"));
			return;
		}
	}

	if (State.AccumDistance + 0.1f >= Req.Distance)
	{
		EndMotion_Internal(FName("End.DistanceReached"));
	}
}

void UJRPGCombatMotionComponent::Tick_TimeElapsed(float DeltaTime)
{
	const FCombatMotionRequest& Req = State.ActiveRequest;

	if (State.Elapsed >= Req.Duration)
	{
		EndMotion_Internal(FName("End.DurationElapsed"));
		return;
	}

	const FVector Dir = Req.Direction.GetSafeNormal();
	const float Speed = Req.Distance / Req.Duration;
	const float Step = Speed * DeltaTime;

	FaceDirectionIfNeeded(Dir, DeltaTime);

	FHitResult Hit;
	SafeMoveStep(Dir * Step, Req.EndPolicy != ECombatMotionEndPolicy::IgnoreBlock, Hit);
	State.AccumDistance += Step;

	if (Hit.IsValidBlockingHit())
	{
		State.bBlocked = true;
		State.LastBlockHit = Hit;
		OnMotionBlocked.Broadcast(State.ActiveHandle, Hit);

		if (Req.Archetype == ECombatMotionArchetype::WallSlam)
		{
			OnWallSlam.Broadcast(State.ActiveHandle, Hit, FName("WallSlam"));
		}

		if (Req.EndPolicy == ECombatMotionEndPolicy::StopOnBlock)
		{
			EndMotion_Internal(Req.Archetype == ECombatMotionArchetype::WallSlam ? FName("End.WallSlam") : FName("End.Blocked"));
			return;
		}
	}
}

void UJRPGCombatMotionComponent::Tick_VelocityCurve(float DeltaTime)
{
	const FCombatMotionRequest& Req = State.ActiveRequest;

	if (State.Elapsed >= Req.Duration)
	{
		EndMotion_Internal(FName("End.DurationElapsed"));
		return;
	}

	const float T = FMath::Clamp(State.Elapsed / Req.Duration, 0.f, 1.f);
	const float Mult = Req.SpeedCurve ? Req.SpeedCurve->GetFloatValue(T) : 1.f;

	const float BaseSpeed = Req.Distance / Req.Duration;
	const float Step = BaseSpeed * Mult * DeltaTime;

	const FVector Dir = Req.Direction.GetSafeNormal();
	FaceDirectionIfNeeded(Dir, DeltaTime);

	FHitResult Hit;
	SafeMoveStep(Dir * Step, Req.EndPolicy != ECombatMotionEndPolicy::IgnoreBlock, Hit);
	State.AccumDistance += Step;

	if (Hit.IsValidBlockingHit())
	{
		State.bBlocked = true;
		State.LastBlockHit = Hit;
		OnMotionBlocked.Broadcast(State.ActiveHandle, Hit);

		if (Req.Archetype == ECombatMotionArchetype::WallSlam)
		{
			OnWallSlam.Broadcast(State.ActiveHandle, Hit, FName("WallSlam"));
		}

		if (Req.EndPolicy == ECombatMotionEndPolicy::StopOnBlock)
		{
			EndMotion_Internal(Req.Archetype == ECombatMotionArchetype::WallSlam ? FName("End.WallSlam") : FName("End.Blocked"));
			return;
		}
	}
}

void UJRPGCombatMotionComponent::Tick_RootMotion(float DeltaTime)
{
	const FCombatMotionRequest& Req = State.ActiveRequest;

	UAnimInstance* Anim = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim || !Req.RootMontage)
	{
		EndMotion_Internal(FName("End.RootMotionInvalid"));
		return;
	}

	if (!Anim->Montage_IsPlaying(Req.RootMontage))
	{
		EndMotion_Internal(FName("End.RootMotionFinished"));
	}
}

bool UJRPGCombatMotionComponent::IsGrounded() const
{
	return CharMove && CharMove->IsMovingOnGround();
}

void UJRPGCombatMotionComponent::Tick_Launch(float DeltaTime)
{
	const FCombatMotionRequest& Req = State.ActiveRequest;

	// 안전 종료: 최대 시간
	if (State.Elapsed >= Req.LaunchMaxTime)
	{
		EndMotion_Internal(FName("End.LaunchTimeout"));
		return;
	}

	// 착지 종료(넉업/슬램)
	if (Req.bEndLaunchWhenGrounded && IsGrounded())
	{
		if (Req.Archetype == ECombatMotionArchetype::SlamToGround)
			EndMotion_Internal(FName("End.SlamGrounded"));
		else
			EndMotion_Internal(FName("End.Grounded"));
		return;
	}
}

void UJRPGCombatMotionComponent::Exec_Teleport()
{
	const FCombatMotionRequest& Req = State.ActiveRequest;
	OwnerCharacter->SetActorLocation(Req.TeleportDest, false, nullptr, ETeleportType::TeleportPhysics);
}

void UJRPGCombatMotionComponent::ApplyZoneClampIfAvailable()
{
	if (!OwnerCharacter) return;

	UCapsuleComponent* Cap = OwnerCharacter->GetCapsuleComponent();
	if (!Cap) return;

	TArray<AActor*> Overlapping;
	Cap->GetOverlappingActors(Overlapping, ACombatZoneActor::StaticClass());

	// 안전: 너무 많으면 상한
	if (Overlapping.Num() > (int32)ZoneClampSearchMaxActors)
	{
		Overlapping.SetNum((int32)ZoneClampSearchMaxActors);
	}

	ACombatZoneActor* Zone = nullptr;
	for (AActor* A : Overlapping)
	{
		Zone = Cast<ACombatZoneActor>(A);
		if (Zone) break;
	}
	if (!Zone) return;

	const float Radius = Cap->GetScaledCapsuleRadius();
	const float HalfH  = Cap->GetScaledCapsuleHalfHeight();

	const FVector Current = OwnerCharacter->GetActorLocation();
	const FVector Clamped = Zone->ClampCharacterLocation(Current, Radius, HalfH);

	if (!Clamped.Equals(Current, 0.1f))
	{
		OwnerCharacter->SetActorLocation(Clamped, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

bool UJRPGCombatMotionComponent::TryDequeueAndStart()
{
	if (State.ActiveHandle.IsValid()) return false;
	if (PendingQueue.Num() == 0) return false;

	int32 BestIdx = INDEX_NONE;
	int32 BestPr = INT32_MIN;

	for (int32 i = 0; i < PendingQueue.Num(); ++i)
	{
		const int32 P = ResolvePriority(PendingQueue[i]);
		if (P > BestPr)
		{
			BestPr = P;
			BestIdx = i;
		}
	}

	if (BestIdx == INDEX_NONE) return false;

	const FCombatMotionRequest Next = PendingQueue[BestIdx];
	PendingQueue.RemoveAtSwap(BestIdx);

	const FCombatMotionHandle H = MakeHandle(Next);
	StartMotion_Internal(Next, H);
	return true;
}
