#include "Combat/Motion/CombatMotionComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Groggy/GroggyComponent.h"

UCombatMotionComponent::UCombatMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;
}

void UCombatMotionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* C = GetCharacterOwner())
	{
		CharMove = C->GetCharacterMovement();
	}
	Groggy = GetOwner() ? GetOwner()->FindComponentByClass<UGroggyComponent>() : nullptr;
}

ACharacter* UCombatMotionComponent::GetCharacterOwner() const
{
	return Cast<ACharacter>(GetOwner());
}

UBattleSessionSubsystem* UCombatMotionComponent::GetBattle() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

int32 UCombatMotionComponent::GetPriorityForType(ECombatMotionType Type) const
{
	switch (Type)
	{
	case ECombatMotionType::GrappleMove: return 300;
	case ECombatMotionType::HitMove: return 200;
	case ECombatMotionType::SkillMove: return 100;
	default:return 0;
	}
}

bool UCombatMotionComponent::ValidateRequest(const FCombatMotionRequest& Req, FName& OutReason) const
{
	OutReason = NAME_None;

	if (!GetOwner())
	{
		OutReason = "Reject.NoOwner";
		return false;
	}

	if (Req.Type == ECombatMotionType::None)
	{
		OutReason = "Reject.InvalidType";
		return false;
	}

	if (Req.ExecMode == ECombatMotionExecMode::VelocityCurve)
	{
		if (Req.Duration <= 0.f && Req.Distance <= 0.f)
		{
			OutReason = "Reject.InvalidVelocityCurveSpec";
			return false;
		}
	}

	if (Req.ExecMode == ECombatMotionExecMode::RootMotion)
	{
		// 외부 몽타주 구동이면 RootMontage가 없어도 추적 가능
	}

	if (Req.ExecMode == ECombatMotionExecMode::Teleport)
	{
		if (Req.TeleportDest.IsNearlyZero() && !Req.bComputeTeleportFromTarget && Req.Distance <= 0.f)
		{
			OutReason = "Reject.InvalidTeleportSpec";
			return false;
		}
	}

	// 문서상 SkillMove는 그로기 중 일반적으로 금지
	if (Req.Type == ECombatMotionType::SkillMove && Groggy.IsValid() && Groggy->bGroggy && !Req.bAllowSkillMoveWhileGroggy)
	{
		OutReason ="Reject.GroggyBlocked";
		return false;
	}

	if (Req.Type == ECombatMotionType::HitMove && Groggy.IsValid() && Groggy->bGroggy && !Req.bAllowHitMoveWhileGroggy)
	{
		OutReason = "Reject.GroggyHitMoveBlocked";
		return false;
	}

	return true;
}

FVector UCombatMotionComponent::MakeDirectionFromTarget(AActor* TargetActor) const
{
	if (!GetOwner() || !TargetActor) return FVector::ZeroVector;

	const FVector From = GetOwner()->GetActorLocation();
	const FVector To = TargetActor->GetActorLocation();
	FVector Dir = (To - From);
	Dir.Z = 0.f;
	return Dir.GetSafeNormal();
}

bool UCombatMotionComponent::ResolveRequestContext(FCombatMotionRequest& InOutReq, FName& OutReason)
{
	OutReason = NAME_None;

	if (InOutReq.bComputeDirectionFromTarget)
	{
		AActor* Target = InOutReq.Target.Get();
		if (!Target)
		{
			OutReason = "Reject.InvalidTarget";
			return false;
		}
		const FVector Dir = MakeDirectionFromTarget(Target);
		if (Dir.IsNearlyZero())
		{
			OutReason = "Reject.InvalidTarget";
			return false;
		}
		InOutReq.Direction = Dir;
	}

	if (InOutReq.ExecMode == ECombatMotionExecMode::Teleport)
	{
		if (InOutReq.bComputeTeleportFromTarget)
		{
			AActor* Target = InOutReq.Target.Get();
			if (!Target)
			{
				OutReason = "Reject.InvalidTarget";
				return false;
			}

			FVector Dir = InOutReq.Direction;
			if (Dir.IsNearlyZero())
			{
				Dir = MakeDirectionFromTarget(Target);
			}
			if (Dir.IsNearlyZero())
			{
				OutReason = "Reject.InvalidTarget";
				return false;
			}

			const FVector TargetLoc = Target->GetActorLocation();
			InOutReq.TeleportDest = TargetLoc - Dir * InOutReq.StopShortDistance;
		}
		else if (InOutReq.TeleportDest.IsNearlyZero() && InOutReq.Distance > 0.f)
		{
			FVector Dir = InOutReq.Direction;
			if (Dir.IsNearlyZero())
			{
				OutReason = "Reject.InvalidDirection";
				return false;
			}
			InOutReq.TeleportDest = GetOwner()->GetActorLocation() + Dir.GetSafeNormal() * InOutReq.Distance;
		}
	}
	else
	{
		if (InOutReq.Direction.IsNearlyZero())
		{
			OutReason = "Reject.InvalidDirection";
			return false;
		}
		InOutReq.Direction.Z = 0.f;
		InOutReq.Direction = InOutReq.Direction.GetSafeNormal();
	}

	return true;
}

bool UCombatMotionComponent::CanReplaceCurrent(const FCombatMotionRequest& NewReq, FName& OutReason) const
{
	OutReason = NAME_None;

	if (!IsMotionActive())
		return true;

	const FCombatMotionRequest& Cur = MotionState.ActiveRequest;

	const int32 CurPriority = Cur.Priority > 0 ? Cur.Priority : GetPriorityForType(Cur.Type);
	const int32 NewPriority = NewReq.Priority > 0 ? NewReq.Priority : GetPriorityForType(NewReq.Type);

	if (NewPriority > CurPriority)
	{
		if (!Cur.bCancelable && !Cur.bCancelOnNewHigherPriority)
		{
			OutReason = "Reject.CurrentNotCancelable";
			return false;
		}
		return true;
	}

	if (NewPriority == CurPriority)
	{
		// same priority -> Last Writer Wins
		if (!Cur.bCancelable)
		{
			OutReason ="Reject.CurrentNotCancelable";
			return false;
		}
		return true;
	}

	OutReason = "Reject.LowerPriority";
	return false;
}

void UCombatMotionComponent::StartAcceptedMotion(const FCombatMotionRequest& Req, const FCombatMotionHandle& Handle, bool bBroadcastReplaced, const FCombatMotionHandle& ReplacedHandle)
{
	MotionState = FCombatMotionState();
	MotionState.ActiveHandle = Handle;
	MotionState.ActiveRequest = Req;
	MotionState.StartTimeReal = FPlatformTime::Seconds();
	MotionState.LastWorldLocation = GetOwner()->GetActorLocation();
	MotionState.ResolvedDirection = Req.Direction;
	MotionState.ResolvedTeleportDest = Req.TeleportDest;

	if (Req.ExecMode == ECombatMotionExecMode::RootMotion && !Req.bMontageDrivenExternally && Req.RootMontage)
	{
		if (ACharacter* C = GetCharacterOwner())
		{
			C->PlayAnimMontage(Req.RootMontage);
		}
	}

	OnCombatMotionStarted.Broadcast(Handle, Req.Type);

	if (bBroadcastReplaced)
	{
		OnCombatMotionReplaced.Broadcast(ReplacedHandle, Handle, "Cancel.ReplacedByHigher");
	}
}

void UCombatMotionComponent::FinishTeleportRequestImmediately(const FCombatMotionRequest& Req, const FCombatMotionHandle& Handle)
{
	bool bClamped = false;
	FVector Dest = Req.TeleportDest;
	if (Req.bAllowClamp)
	{
		Dest = ClampLocationToBattle(Dest, bClamped);
	}

	GetOwner()->SetActorLocation(Dest, false);
	MotionState = FCombatMotionState();
	MotionState.ActiveHandle = Handle;
	MotionState.ActiveRequest = Req;
	MotionState.bIsClampedThisFrame = bClamped;

	OnCombatMotionStarted.Broadcast(Handle, Req.Type);
	OnCombatMotionEnded.Broadcast(Handle, "End.TeleportApplied");

	MotionState = FCombatMotionState();
}

FCombatMotionResponse UCombatMotionComponent::RequestCombatMotion(const FCombatMotionRequest& InReq)
{
	FCombatMotionRequest Req = InReq;

	if (Req.Priority <= 0)
	{
		Req.Priority = GetPriorityForType(Req.Type);
	}
	if (Req.OwnerTag.IsNone())
	{
		Req.OwnerTag = "Motion";
	}

	FName Reason = NAME_None;
	if (!ValidateRequest(Req, Reason))
	{
		return FCombatMotionResponse::Make(ECombatMotionResult::Rejected, FCombatMotionHandle(), Reason);
	}

	if (!ResolveRequestContext(Req, Reason))
	{
		return FCombatMotionResponse::Make(ECombatMotionResult::Rejected, FCombatMotionHandle(), Reason);
	}

	const bool bHadActive = IsMotionActive();
	FCombatMotionHandle OldHandle = MotionState.ActiveHandle;

	if (bHadActive)
	{
		if (!CanReplaceCurrent(Req, Reason))
		{
			return FCombatMotionResponse::Make(ECombatMotionResult::Rejected, FCombatMotionHandle(), Reason);
		}

		OnCombatMotionEnded.Broadcast(MotionState.ActiveHandle, "Cancel.ReplacedByHigher");
		MotionState = FCombatMotionState();
	}

	FCombatMotionHandle NewHandle;
	NewHandle.OwnerTag = Req.OwnerTag;
	NewHandle.UniqueId = NextMotionId++;

	if (Req.ExecMode == ECombatMotionExecMode::Teleport)
	{
		FinishTeleportRequestImmediately(Req, NewHandle);
		return FCombatMotionResponse::Make(
			bHadActive ? ECombatMotionResult::ReplacedExisting : ECombatMotionResult::Accepted,
			NewHandle,
			bHadActive ? "Accept.Replace" : "Accept.Teleport");
	}

	StartAcceptedMotion(Req, NewHandle, bHadActive, OldHandle);

	return FCombatMotionResponse::Make(
		bHadActive ? ECombatMotionResult::ReplacedExisting : ECombatMotionResult::Accepted,
		NewHandle,
		bHadActive ? "Accept.Replace" : "Accept");
}

bool UCombatMotionComponent::CancelCombatMotion(FCombatMotionHandle Handle, FName ReasonTag)
{
	if (!IsMotionActive()) return false;
	if (!(MotionState.ActiveHandle == Handle)) return false;

	EndActiveMotion(ReasonTag.IsNone() ? "Cancel.Explicit" : ReasonTag);
	return true;
}

int32 UCombatMotionComponent::CancelAllByOwner(FName OwnerTag, FName ReasonTag)
{
	if (!IsMotionActive()) return 0;
	if (MotionState.ActiveHandle.OwnerTag != OwnerTag) return 0;

	EndActiveMotion(ReasonTag.IsNone() ? "Cancel.ByOwner" : ReasonTag);
	return 1;
}

void UCombatMotionComponent::EndActiveMotion(FName EndReasonTag)
{
	if (!IsMotionActive()) return;

	const FCombatMotionHandle Handle = MotionState.ActiveHandle;
	if (CharMove.IsValid())
	{
		CharMove->StopMovementImmediately();
	}

	OnCombatMotionEnded.Broadcast(Handle, EndReasonTag);
	MotionState = FCombatMotionState();
}

FVector UCombatMotionComponent::ClampLocationToBattle(const FVector& InLocation, bool& bWasClamped) const
{
	bWasClamped = false;

	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Battle) return InLocation;

	FVector Center;
	float Radius = 0.f;
	if (!Battle->GetCombatClamp(Center, Radius) || Radius <= 0.f)
	{
		return InLocation;
	}

	FVector Delta = InLocation - Center;
	const float Z = Delta.Z;
	Delta.Z = 0.f;

	const float Dist2D = Delta.Size();
	if (Dist2D <= Radius)
	{
		return InLocation;
	}

	bWasClamped = true;
	const FVector Clamped2D = Delta.GetSafeNormal() * Radius;
	return FVector(Center.X + Clamped2D.X, Center.Y + Clamped2D.Y, InLocation.Z + 0.f * Z);
}

void UCombatMotionComponent::ApplyClampIfNeeded()
{
	if (!IsMotionActive()) return;
	if (!MotionState.ActiveRequest.bAllowClamp) return;

	bool bClamped = false;
	const FVector Cur = GetOwner()->GetActorLocation();
	const FVector NewLoc = ClampLocationToBattle(Cur, bClamped);

	MotionState.bIsClampedThisFrame = bClamped;
	if (bClamped)
	{
		GetOwner()->SetActorLocation(NewLoc, false);

		if (MotionState.ActiveRequest.EndPolicy == ECombatMotionEndPolicy::HitWallOrBlocked)
		{
			EndActiveMotion("End.ClampHit");
		}
	}
}

void UCombatMotionComponent::TickVelocityCurve(float DeltaTime)
{
	const FCombatMotionRequest& Req = MotionState.ActiveRequest;
	if (!GetOwner()) return;

	MotionState.Elapsed += DeltaTime;

	float EvalX = MotionState.Elapsed;
	if (Req.Duration > 0.f)
	{
		EvalX = FMath::Clamp(MotionState.Elapsed / Req.Duration, 0.f, 1.f);
	}

	float Speed = 0.f;
	if (Req.SpeedCurve)
	{
		Speed = Req.SpeedCurve->GetFloatValue(EvalX);
	}
	else
	{
		if (Req.Duration > 0.f && Req.Distance > 0.f)
		{
			Speed = Req.Distance / Req.Duration;
		}
		else
		{
			Speed = 600.f;
		}
	}

	const FVector Delta = MotionState.ResolvedDirection * Speed * DeltaTime;

	FHitResult Hit;
	const FVector Before = GetOwner()->GetActorLocation();
	GetOwner()->AddActorWorldOffset(Delta, true, &Hit);
	const FVector After = GetOwner()->GetActorLocation();

	const float Moved = FVector::Dist2D(Before, After);
	MotionState.AccumulatedDistance += Moved;
	MotionState.LastWorldLocation = After;

	if (Hit.IsValidBlockingHit())
	{
		MotionState.bBlocked = true;
		MotionState.LastBlockHitResult = Hit;
		OnCombatMotionBlocked.Broadcast(MotionState.ActiveHandle, Hit);

		if (Req.EndPolicy == ECombatMotionEndPolicy::HitWallOrBlocked)
		{
			EndActiveMotion("End.HitWall");
			return;
		}
	}

	ApplyClampIfNeeded();
	if (!IsMotionActive()) return;

	if (Req.EndPolicy == ECombatMotionEndPolicy::DistanceReached && Req.Distance > 0.f && MotionState.AccumulatedDistance >= Req.Distance)
	{
		EndActiveMotion("End.DistanceReached");
		return;
	}

	if (Req.EndPolicy == ECombatMotionEndPolicy::TimeElapsed && Req.Duration > 0.f && MotionState.Elapsed >= Req.Duration)
	{
		EndActiveMotion("End.DurationElapsed");
		return;
	}
}

void UCombatMotionComponent::TickRootMotion(float DeltaTime)
{
	MotionState.Elapsed += DeltaTime;
	ApplyClampIfNeeded();
	if (!IsMotionActive()) return;

	const FCombatMotionRequest& Req = MotionState.ActiveRequest;

	if (Req.EndPolicy == ECombatMotionEndPolicy::MontageEnded)
	{
		bool bStillPlaying = false;

		if (ACharacter* C = GetCharacterOwner())
		{
			if (USkeletalMeshComponent* Mesh = C->GetMesh())
			{
				if (UAnimInstance* Anim = Mesh->GetAnimInstance())
				{
					if (Req.RootMontage)
					{
						bStillPlaying = Anim->Montage_IsPlaying(Req.RootMontage);
					}
					else
					{
						bStillPlaying = Anim->IsAnyMontagePlaying();
					}
				}
			}
		}

		if (!bStillPlaying)
		{
			EndActiveMotion("End.MontageEnded");
			return;
		}
	}

	if (Req.EndPolicy == ECombatMotionEndPolicy::TimeElapsed && Req.Duration > 0.f && MotionState.Elapsed >= Req.Duration)
	{
		EndActiveMotion("End.DurationElapsed");
		return;
	}
}

void UCombatMotionComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	if (!IsMotionActive()) return;

	switch (MotionState.ActiveRequest.ExecMode)
	{
	case ECombatMotionExecMode::VelocityCurve:
		TickVelocityCurve(DeltaTime);
		break;
	
	case ECombatMotionExecMode::RootMotion:
		TickRootMotion(DeltaTime);
		break;
	
	case ECombatMotionExecMode::Teleport:
	default:
		break;
	}
}