#include "Combat/Presentation/CombatPresentationComponent.h"

#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Items/CombatItemExecutionSubsystem.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatPlayerController.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Skills/SkillDataAsset.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"

#include "Combat/Motion/CombatMotionComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Stats/HPComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

UCombatPresentationComponent::UCombatPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}

void UCombatPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	SkillComp = GetOwner() ?GetOwner()->FindComponentByClass<USkillComponent>() : nullptr;
	CharacterComp = GetOwner() ?GetOwner()->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
}

UBattleSessionSubsystem* UCombatPresentationComponent::GetBattle()const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

UTacticalModeSubsystem* UCombatPresentationComponent::GetTactical() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UTacticalModeSubsystem>() : nullptr;
}


void UCombatPresentationComponent::TickComponent(float, ELevelTick, FActorComponentTickFunction*)
{
	if (!GetOwner())
	{
		return;
	}

	if (HasActivePresentation())
	{
		const double Now = FPlatformTime::Seconds();
		if (Now <= Active.FaceTargetUntilRealSec)
		{
			FaceActiveTarget();
		}

		if (!Active.bResolved && Active.AutoResolveAtRealSec > 0.0 && Now >= Active.AutoResolveAtRealSec)
		{
			ResolveActivePresentation();
		}
		if (HasActivePresentation() && Active.AutoFinishAtRealSec > 0.0 && Now >= Active.AutoFinishAtRealSec)
		{
			FinishActivePresentation();
		}
		return;
	}

	TryConsumeTacticalReservation();
}

void UCombatPresentationComponent::TryConsumeTacticalReservation()
{
	UTacticalModeSubsystem* Tactical = GetTactical();
	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Tactical||!Battle)
		return;
	if (!Battle->CanActorExecuteAction(GetOwner()))
		return;
	
	FJRPGTacticalReservation R;
	if (!Tactical->GetReservation(GetOwner(),R)) 
		return;

	TArray<AActor*> Targets;
	for (const TWeakObjectPtr<AActor> &W : R.Targets)
	{
		if (AActor *A = W.Get())
			Targets.Add(A);
	}

	const FSkillCastResult Result = TryPresentSkill(R.SkillId, Targets,true);
	if (Result.bOk)
	{
		Tactical->ClearReservation(GetOwner());
	}
}

bool UCombatPresentationComponent::TryStartMotionForBasicAttack()
{
	if (!CharacterComp.IsValid() || !CharacterComp->CharacterDef) return true;
	if (!CharacterComp->CharacterDef->bHasBasicAttackMotion) return true;

	UCombatMotionComponent* Motion = GetOwner() ? GetOwner()->FindComponentByClass<UCombatMotionComponent>() : nullptr;
	if (!Motion) return false;

	FJRPGCombatMotionRequest Req = CharacterComp->CharacterDef->BasicAttackMotion;
	if (Active.Montage && Active.Montage->HasRootMotion() && Req.ExecMode != EJRPGCombatMotionExecMode::RootMotion)
	{
		UE_LOG(LogTemp, Log, TEXT("[Presentation][RootMotion] SkipCodeMotion Owner=%s Action=BasicAttack Reason=MontageHasRootMotion"), *GetNameSafe(GetOwner()));
		return true;
	}

	Req.Instigator = GetOwner();
	if (Active.Targets.Num() > 0) Req.Target = Active.Targets[0];
	Req.OwnerTag = "BasicAttack";

	if (Req.ExecMode == EJRPGCombatMotionExecMode::RootMotion && Req.RootMontage == nullptr)
	{
		Req.RootMontage = CharacterComp->CharacterDef->BasicAttackMontage;
		Req.bMontageDrivenExternally = true;
	}

	const FJRPGCombatMotionResponse Resp = Motion->RequestCombatMotion(Req);
	if (Resp.Result == EJRPGCombatMotionResult::Accepted || Resp.Result == EJRPGCombatMotionResult::ReplacedExisting)
	{
		Active.MotionHandle = Resp.Handle;
		Active.bHasMotion = true;
		return true;
	}
	return false;
}

bool UCombatPresentationComponent::TryStartMotionForSkill(USkillDataAsset* SkillDef)
{
	if (!SkillDef || !SkillDef->bHasSkillMotion) return true;

	UCombatMotionComponent* Motion = GetOwner() ? GetOwner()->FindComponentByClass<UCombatMotionComponent>() : nullptr;
	if (!Motion) return false;

	FJRPGCombatMotionRequest Req = SkillDef->SkillMotion;
	if (Active.Montage && Active.Montage->HasRootMotion() && Req.ExecMode != EJRPGCombatMotionExecMode::RootMotion)
	{
		UE_LOG(LogTemp, Log, TEXT("[Presentation][RootMotion] SkipCodeMotion Owner=%s Action=Skill.%s Reason=MontageHasRootMotion"), *GetNameSafe(GetOwner()), *SkillDef->SkillId.ToString());
		return true;
	}

	Req.Instigator = GetOwner();
	if (Active.Targets.Num() > 0) Req.Target = Active.Targets[0];
	Req.OwnerTag = SkillDef->SkillId;

	if (Req.ExecMode == EJRPGCombatMotionExecMode::RootMotion && Req.RootMontage == nullptr)
	{
		Req.RootMontage = SkillDef->CastMontage;
		Req.bMontageDrivenExternally = true;
	}

	const FJRPGCombatMotionResponse Resp = Motion->RequestCombatMotion(Req);
	if (Resp.Result == EJRPGCombatMotionResult::Accepted || Resp.Result == EJRPGCombatMotionResult::ReplacedExisting)
	{
		Active.MotionHandle = Resp.Handle;
		Active.bHasMotion = true;
		return true;
	}
	return false;
}

void UCombatPresentationComponent::CancelActiveMotionIfNeeded()
{
	if (!Active.bHasMotion) return;

	if (UCombatMotionComponent* Motion = GetOwner() ? GetOwner()->FindComponentByClass<UCombatMotionComponent>() : nullptr)
	{
		Motion->CancelCombatMotion(Active.MotionHandle, "Cancel.Presentation");
	}
}

void UCombatPresentationComponent::StopActiveMontageIfNeeded(float BlendOutTime)
{
	if (!Active.Montage)
	{
		return;
	}

	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* MeshComp = CharacterOwner ? CharacterOwner->GetMesh() : nullptr;
	UAnimInstance* Anim = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!Anim || !Anim->Montage_IsPlaying(Active.Montage))
	{
		return;
	}

	FOnMontageEnded EmptyEndDelegate;
	Anim->Montage_SetEndDelegate(EmptyEndDelegate, Active.Montage);
	Anim->Montage_Stop(FMath::Max(0.f, BlendOutTime), Active.Montage);
}

void UCombatPresentationComponent::AcquireInputLockForPresentation()
{
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (ULocomotionComponent* Loco = P->FindComponentByClass<ULocomotionComponent>())
		{
			const TJRPGResult<FJRPGHandle> Result = Loco->AcquireInputLock("Present.Action");
			if (Result.bOk)
			{
				Active.InputLockHandle = Result.Value;
				Active.bHasInputLock = true;
			}
		}
	}
}

void UCombatPresentationComponent::ReleaseInputLockForPresentation()
{
	if (!Active.bHasInputLock)
		return;

	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (ULocomotionComponent* Loco = P->FindComponentByClass<ULocomotionComponent>())
		{
			Loco->ReleaseInputLock(Active.InputLockHandle);
			Active.bHasInputLock = false;
		}
	}
}

void UCombatPresentationComponent::StopPathFollowingForPresentation(FName ReasonTag)
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	AAIController* AIController = CharacterOwner ? Cast<AAIController>(CharacterOwner->GetController()) : nullptr;
	if (!CharacterOwner)
	{
		return;
	}

	const EPathFollowingStatus::Type PreviousStatus = AIController ? AIController->GetMoveStatus() : EPathFollowingStatus::Idle;
	if (AIController)
	{
		AIController->StopMovement();
	}
	if (UCharacterMovementComponent* MoveComp = CharacterOwner->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	UE_LOG(LogTemp, Log, TEXT("[Presentation][StopMovement] Owner=%s Reason=%s PreviousPathFollowingStatus=%d"),
		*GetNameSafe(GetOwner()),
		ReasonTag.IsNone() ? TEXT("None") : *ReasonTag.ToString(),
		(int32)PreviousStatus);
}

void UCombatPresentationComponent::ApplyPresentationMovementSlowIfNeeded()
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
	if (!MoveComp || Active.bHasMovementSlow)
	{
		return;
	}

	Active.SavedMaxWalkSpeed = MoveComp->MaxWalkSpeed;
	const float SlowMultiplier = Active.Type == EPresentedCombatActionType::BasicAttack ? 0.30f : 0.20f;
	MoveComp->MaxWalkSpeed = FMath::Max(10.f, Active.SavedMaxWalkSpeed * SlowMultiplier);
	Active.bHasMovementSlow = true;
	UE_LOG(LogTemp, Log, TEXT("[Presentation] AttackMoveSpeedSlow Owner=%s Action=%s MaxWalkSpeed=%.1f SlowMaxWalkSpeed=%.1f"),
		*GetNameSafe(GetOwner()), *Active.ActionId.ToString(), Active.SavedMaxWalkSpeed, MoveComp->MaxWalkSpeed);
}

void UCombatPresentationComponent::RestorePresentationMovementSlowIfNeeded()
{
	if (!Active.bHasMovementSlow)
	{
		return;
	}

	if (ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = CharacterOwner->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = Active.SavedMaxWalkSpeed;
		}
	}

	Active.bHasMovementSlow = false;
}

void UCombatPresentationComponent::ApplyAttackRotationLockIfNeeded()
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
	if (!CharacterOwner || !MoveComp || Active.bHasRotationLock)
	{
		return;
	}

	Active.bSavedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
	Active.bSavedUseControllerDesiredRotation = MoveComp->bUseControllerDesiredRotation;
	Active.bSavedUseControllerRotationYaw = CharacterOwner->bUseControllerRotationYaw;
	Active.bHasRotationLock = true;

	MoveComp->bOrientRotationToMovement = false;
	MoveComp->bUseControllerDesiredRotation = false;
	CharacterOwner->bUseControllerRotationYaw = false;
	Active.FaceTargetUntilRealSec = FPlatformTime::Seconds() + 0.12;
	FaceActiveTarget();
}

void UCombatPresentationComponent::RestoreAttackRotationLockIfNeeded()
{
	if (!Active.bHasRotationLock)
	{
		return;
	}

	if (ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = CharacterOwner->GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = Active.bSavedOrientRotationToMovement;
			MoveComp->bUseControllerDesiredRotation = Active.bSavedUseControllerDesiredRotation;
		}
		CharacterOwner->bUseControllerRotationYaw = Active.bSavedUseControllerRotationYaw;
	}

	Active.bHasRotationLock = false;
	Active.FaceTargetUntilRealSec = 0.0;

}

void UCombatPresentationComponent::FaceActiveTarget()
{
	AActor* OwnerActor = GetOwner();
	AActor* Target = Active.Targets.Num() > 0 ? Active.Targets[0].Get() : nullptr;
	if (!IsValid(OwnerActor) || !IsValid(Target))
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - OwnerActor->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	OwnerActor->SetActorRotation(ToTarget.Rotation());
}

void UCombatPresentationComponent::EmitCue(FName CueTag)
{
	if (CueTag.IsNone())
		return;

	FCombatCueEvent Evt;
	Evt.OwnerActor =GetOwner();
	Evt.CueTag =CueTag;
	Evt.ContextTag =Active.ActionId;
	OnCombatCueEvent.Broadcast(Evt);
}

void UCombatPresentationComponent::ConfigureAutoPresentationTiming(bool bNoPlayableMontage)
{
	Active.AutoResolveAtRealSec = 0.0;
	Active.AutoFinishAtRealSec = 0.0;

	if (bNoPlayableMontage || !Active.Montage || Active.ResolveTiming == ECombatResolveTiming::Immediate)
	{
		Active.AutoResolveAtRealSec = Active.StartedAtRealSec;
		Active.AutoFinishAtRealSec = Active.StartedAtRealSec;
		return;
	}

	// AnimNotifyWindow and MontageEnded presentations are driven by animation notifies / montage end delegates.
	// Timers are intentionally not armed here so damage and unlock do not drift from the actual montage.
}

void UCombatPresentationComponent::PlayActiveMontageOrResolve()
{
	EmitCue(Active.StartCueTag);
	Active.StartedAtRealSec = FPlatformTime::Seconds();

	ConfigureAutoPresentationTiming(!Active.Montage);

	if (Active.ResolveTiming == ECombatResolveTiming::Immediate || !Active.Montage)
	{
		ResolveActivePresentation();
		if (HasActivePresentation())
		{
			FinishActivePresentation();
		}
		return;
	}

	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		if (C->CustomTimeDilation <= 0.06f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Presentation][TimeDilationReset] Owner=%s Action=%s PreviousDilation=%.3f"),
				*GetNameSafe(GetOwner()),
				*Active.ActionId.ToString(),
				C->CustomTimeDilation);
			C->CustomTimeDilation = 1.f;
		}
		if (USkeletalMeshComponent* MeshComp = C->GetMesh())
		{
			if (MeshComp->GlobalAnimRateScale <= 0.1f)
			{
				MeshComp->GlobalAnimRateScale = 1.f;
			}
		}

		const float PlayedLen = C->PlayAnimMontage(Active.Montage, 1.f);
		if (PlayedLen > 0.f)
		{
			if (USkeletalMeshComponent* MeshComp = C->GetMesh())
			{
				if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
				{
					FOnMontageEnded EndDelegate;
					EndDelegate.BindUObject(this, &UCombatPresentationComponent::HandleActiveMontageEnded);
					Anim->Montage_SetEndDelegate(EndDelegate, Active.Montage);
				}
			}
			return;
		}
	}

	ConfigureAutoPresentationTiming(true);
	ResolveActivePresentation();
	if (HasActivePresentation())
	{
		FinishActivePresentation();
	}
}

void UCombatPresentationComponent::HandleActiveMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!HasActivePresentation() || Montage != Active.Montage)
	{
		return;
	}

	if (bInterrupted)
	{
		CancelActivePresentation("Cancel.MontageInterrupted", Active.Type == EPresentedCombatActionType::Skill);
		return;
	}

	if (!Active.bResolved && Active.ResolveTiming == ECombatResolveTiming::MontageEnded)
	{
		ResolveActivePresentation();
	}

	if (HasActivePresentation())
	{
		FinishActivePresentation();
	}
}

bool UCombatPresentationComponent::IsAutoAttackSuppressed() const
{
	return FPlatformTime::Seconds() < AutoAttackSuppressedUntilRealSec;
}

void UCombatPresentationComponent::SetAutoAttackSuppressedFor(float DurationSec)
{
	AutoAttackSuppressedUntilRealSec = FMath::Max(AutoAttackSuppressedUntilRealSec, FPlatformTime::Seconds() + FMath::Max(0.f, DurationSec));
}

void UCombatPresentationComponent::ClearAutoAttackSuppression()
{
	AutoAttackSuppressedUntilRealSec = 0.0;
}

float UCombatPresentationComponent::GetMinBasicAttackStartInterval() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsPlayerControlled())
	{
		return 0.75f;
	}

	if (const ICombatParticipantInterface* Participant = Cast<ICombatParticipantInterface>(GetOwner()))
	{
		if (Participant->GetCombatTeam() == ECombatTeam::Enemy)
		{
			return 1.65f;
		}
	}

	return 1.0f;
}

float UCombatPresentationComponent::GetRemainingBasicAttackStartCooldown() const
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return FMath::Max(0.f, static_cast<float>((LastBasicAttackStartWorldTime + GetMinBasicAttackStartInterval()) - Now));
}

FCombatActionResult UCombatPresentationComponent::TryPresentBasicAttack(AActor *Target)
{
	UBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle)
		return FCombatActionResult::Fail("Reject.NoBattleSession");

	if (IsAutoAttackSuppressed())
	{
		const double Remaining = FMath::Max(0.0, AutoAttackSuppressedUntilRealSec - FPlatformTime::Seconds());
		UE_LOG(LogTemp, Log, TEXT("[InputPriority] AutoAttack suppressed by skill input Remaining=%.2f"), Remaining);
		return FCombatActionResult::Fail("Reject.AutoAttackSuppressed");
	}

	if (!IsValid(Target))
	{
		return FCombatActionResult::Fail("Reject.InvalidTarget");
	}
	if (const UHPComponent* TargetHP = Target->FindComponentByClass<UHPComponent>(); TargetHP && TargetHP->IsDead())
	{
		return FCombatActionResult::Fail("Reject.TargetDead");
	}

	const double NowTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float MinInterval = GetMinBasicAttackStartInterval();
	const float DeltaSinceLastStart = static_cast<float>(NowTime - LastBasicAttackStartWorldTime);
	if (DeltaSinceLastStart < MinInterval)
	{
		UE_LOG(LogTemp, Log, TEXT("[AttackTempo] Owner=%s LastAttackStart=%.2f CurrentAttackStart=%.2f Delta=%.2f MinInterval=%.2f Allowed=false"),
			*GetNameSafe(GetOwner()), LastBasicAttackStartWorldTime, NowTime, DeltaSinceLastStart, MinInterval);
		return FCombatActionResult::Fail("Reject.BasicAttackTempoCooldown");
	}
	
	if (!Battle->BeginPresentedAction(GetOwner(),"Present.BasicAttack"))
		return FCombatActionResult::Fail("Reject.CannotPresentAction");

	if (!CharacterComp.IsValid() || !CharacterComp->CharacterDef)
	{
		Battle->AbortPresentedAction(GetOwner(),"Reject.NoCharacterDef");
		return FCombatActionResult::Fail("Reject.NoCharacterDef");
	}

	Active = FActivePresentationState();
	Active.Type = EPresentedCombatActionType::BasicAttack;
	Active.ActionId = "BasicAttack";
	Active.Targets.Add(Target);
	Active.ResolveTiming = CharacterComp->CharacterDef->BasicAttackResolveTiming;
	Active.Montage = CharacterComp->CharacterDef->BasicAttackMontage;
	Active.StartCueTag = CharacterComp->CharacterDef->BasicAttackStartCueTag;
	Active.HitCueTag = CharacterComp->CharacterDef->BasicAttackHitCueTag;
	Active.FinishCueTag = CharacterComp->CharacterDef->BasicAttackFinishCueTag;

	if (!TryStartMotionForBasicAttack())
	{
		Battle->AbortPresentedAction(GetOwner(), "Reject.BasicAttackMotionFailed");
		ClearActiveState();
		return FCombatActionResult::Fail("Reject.BasicAttackMotionFailed");
	}

	UE_LOG(LogTemp, Log, TEXT("[AttackTempo] Owner=%s LastAttackStart=%.2f CurrentAttackStart=%.2f Delta=%.2f MinInterval=%.2f Allowed=true"),
		*GetNameSafe(GetOwner()), LastBasicAttackStartWorldTime, NowTime, DeltaSinceLastStart, MinInterval);
	LastBasicAttackStartWorldTime = NowTime;
	
	StopPathFollowingForPresentation("BeforeBasicAttack");
	OnPresentationStarted.Broadcast(Active.Type, Active.ActionId);
	ApplyPresentationMovementSlowIfNeeded();
	ApplyAttackRotationLockIfNeeded();
	AcquireInputLockForPresentation();
	PlayActiveMontageOrResolve();
	
	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Presentation,
			"Present.BasicAttack",
			FString::Printf(TEXT("Basic attack presentation started")),
			GetOwner(),
			Target,
			FLinearColor(0.8f, 0.9f, 1.f));
	}
	
	return FCombatActionResult::Ok();
}

FSkillCastResult UCombatPresentationComponent::TryPresentSkill(FName SkillId, const TArray<AActor*> &Targets, bool bFromTacticalReservation)
{
	UBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle)
		return FSkillCastResult::Fail("Reject.NoBattleSession");
	
	if (!SkillComp.IsValid())
		return FSkillCastResult::Fail("Reject.NoSkillComponent");
	
	if (!Battle->BeginPresentedAction(GetOwner(), "Present.Skill"))
		return FSkillCastResult::Fail("Reject.CannotPresentAction");

	USkillDataAsset*Def =SkillComp->GetSkillDef(SkillId);
	if (!Def)
	{
		Battle->AbortPresentedAction(GetOwner(),"Reject.SkillNotFound");
		return FSkillCastResult::Fail("Reject.SkillNotFound");
	}

	const FSkillCastResult Prep = SkillComp->PrepareSkillCast(SkillId,Targets,bFromTacticalReservation, "Present.Skill");
	if (!Prep.bOk)
	{
		Battle->AbortPresentedAction(GetOwner(), Prep.ReasonTag);
		return Prep;
	}

	Active =FActivePresentationState();
	Active.Type = EPresentedCombatActionType::Skill;
	Active.ActionId =SkillId;
	Active.ResolveTiming =Def->ResolveTiming;
	Active.Montage =Def->CastMontage;
	Active.StartCueTag =Def->StartCueTag;
	Active.HitCueTag =Def->HitCueTag;
	Active.FinishCueTag =Def->FinishCueTag;
	Active.bFromTacticalReservation =bFromTacticalReservation;

	for (AActor *T : Targets)
	{
		if (T)
			Active.Targets.Add(T);
	}

	if (!TryStartMotionForSkill(Def))
	{
		SkillComp->CancelPreparedSkillCast(true, "Reject.SkillMotionFailed");
		Battle->AbortPresentedAction(GetOwner(), "Reject.SkillMotionFailed");
		ClearActiveState();
		return FSkillCastResult::Fail("Reject.SkillMotionFailed");
	}
	
	StopPathFollowingForPresentation("BeforeSkill");
	OnPresentationStarted.Broadcast(Active.Type, Active.ActionId);
	ApplyPresentationMovementSlowIfNeeded();
	ApplyAttackRotationLockIfNeeded();
	AcquireInputLockForPresentation();
	PlayActiveMontageOrResolve();

	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Presentation,
			"Present.Skill",
			FString::Printf(TEXT("Skill presentation started | Skill=%s Tactical=%s"),
				*SkillId.ToString(),
				bFromTacticalReservation ? TEXT("true") :TEXT("false")),
				GetOwner(),
				Targets.Num() > 0 ? Targets[0] : nullptr,
			FLinearColor(0.7f, 1.f, 1.f));
	}
	
	return FSkillCastResult::Ok();
}

FCombatItemUseResult UCombatPresentationComponent::TryPresentItem(FName ItemId,const TArray<AActor*> &Targets)
{
	UBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle)
		return FCombatItemUseResult::Fail("Reject.NoBattleSession");
	
	if (!Battle->BeginPresentedAction(GetOwner(), "Present.Item"))
		return FCombatItemUseResult::Fail("Reject.CannotPresentAction");

	Active = FActivePresentationState();
	Active.Type = EPresentedCombatActionType::Item;
	Active.ActionId = ItemId;
	Active.ResolveTiming = ECombatResolveTiming::MontageEnded;
	Active.StartCueTag = "Item.Start";
	Active.HitCueTag = "Item.Use";
	Active.FinishCueTag = "Item.Finish";

	for (AActor *T : Targets)
	{
		if (T)
			Active.Targets.Add(T);
	}

	OnPresentationStarted.Broadcast(Active.Type, Active.ActionId);
	AcquireInputLockForPresentation();
	PlayActiveMontageOrResolve();

	if (UCombatDebugSubsystem*Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Presentation,
			"Present.Item",
			FString::Printf(TEXT("Item presentation started | Item=%s"), *ItemId.ToString()),
			GetOwner(),
			Targets.Num() > 0 ? Targets[0] : nullptr,
			FLinearColor(1.f, 1.f, 0.7f));
	}
	
	return FCombatItemUseResult::Ok();
}

void UCombatPresentationComponent::ResolveActivePresentation()
{
	if (!HasActivePresentation())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Presentation] SkipResolve Owner=%s Reason=NoActivePresentation"), *GetNameSafe(GetOwner()));
		return;
	}
	if (Active.bResolved)
	{
		return;
	}
	
	if (Active.Type != EPresentedCombatActionType::Item)
	{
		if (Active.Targets.Num() <= 0 || !IsValid(Active.Targets[0].Get()))
		{
			CancelActivePresentation("Reject.InvalidTarget", Active.Type == EPresentedCombatActionType::Skill);
			return;
		}

		if (const UHPComponent* TargetHP = Active.Targets[0]->FindComponentByClass<UHPComponent>(); TargetHP && TargetHP->IsDead())
		{
			CancelActivePresentation("Reject.TargetDead", Active.Type == EPresentedCombatActionType::Skill);
			return;
		}
	}
	
		UBattleSessionSubsystem * Battle = GetBattle();
	if (!Battle || !Battle->CanActorResolvePresentedAction(GetOwner()))
		 return;
	

	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Presentation,
			"Present.Resolve",
			FString::Printf(TEXT("Resolve active presentation | Type=%d Action=%s"),
					(int32)Active.Type,
					*Active.ActionId.ToString()),
			GetOwner(),
			Active.Targets.Num() > 0 ? Active.Targets[0].Get() : nullptr,
			FLinearColor(1.f, 0.9f, 0.6f));
	}

	EmitCue(Active.HitCueTag);

	switch (Active.Type)
	{
	case EPresentedCombatActionType::BasicAttack:
		{
			if (!CharacterComp.IsValid()||!CharacterComp->CharacterDef)
			{
				CancelActivePresentation("Reject.NoCharacterDef", false);
				return;
			}
			if (Active.Targets.Num()<=0)
			{
				CancelActivePresentation("Reject.InvalidTarget", false);
				return;
			}

			const EPresentedCombatActionType ResolvingType = Active.Type;
			const FName ResolvingActionId = Active.ActionId;
			const TWeakObjectPtr<AActor> ResolvingTarget = Active.Targets[0];
			AActor* Target = ResolvingTarget.Get();
			if (!IsValid(Target))
			{
				CancelActivePresentation("Reject.InvalidTarget", false);
				return;
			}

			FBasicAttackRequest Req;
			Req.Attacker =GetOwner();
			Req.Target =Target;
			Req.BasePower =CharacterComp->CharacterDef->BasicAttackBasePower;
			Req.AttackScale =CharacterComp->CharacterDef->BasicAttackAttackScale;
			Req.DefenseScale = CharacterComp->CharacterDef->BasicAttackDefenseScale;
			Req.APCost = CharacterComp->CharacterDef->BasicAttackAPCost;
			Req.SPGainOnHit = CharacterComp->CharacterDef->BasicAttackSPGainOnHit;
			Req.SPGainOnKill = CharacterComp->CharacterDef->BasicAttackSPGainOnKill;
			Req.GroggyPower = CharacterComp->CharacterDef->BasicAttackGroggyPower;
			Req.ThreatMultiplier = CharacterComp->CharacterDef->BasicAttackThreatMultiplier;
			Req.ReasonTag = "Present.BasicAttack";

			if (UBasicCombatSubsystem *Basic = GetWorld()->GetSubsystem<UBasicCombatSubsystem>())
			{
				const FCombatActionResult Result = Basic->ExecuteBasicAttack(Req);
				if (!IsValid(this) || !IsValid(GetOwner()))
				{
					return;
				}

				if (UBattleSessionSubsystem* BattleAfterExecute = GetBattle())
				{
					UE_LOG(LogTemp, Warning,
						TEXT("CombatPresentationComponent::ResolveActivePresentation : basic attack executed | Owner=%s OwnerValid=%s TargetValid=%s BattleActive=%s Phase=%d"),
						*GetNameSafe(GetOwner()),
						IsValid(GetOwner()) ? TEXT("true") : TEXT("false"),
						IsValid(Target) ? TEXT("true") : TEXT("false"),
						BattleAfterExecute->IsBattleActive() ? TEXT("true") : TEXT("false"),
						(int32)BattleAfterExecute->GetPhase());
				}

				if (HasActivePresentation()
					&& Active.Type == ResolvingType
					&& Active.ActionId == ResolvingActionId
					&& Active.Targets.Num() > 0
					&& Active.Targets[0] == ResolvingTarget)
				{
					if (!Result.bOk)
					{
						CancelActivePresentation(Result.ReasonTag.IsNone() ? "Reject.ResolveFailed" : Result.ReasonTag, false);
						return;
					}
					
					Active.bResolved = Result.bOk;
					if (Result.bOk)
					{
						UE_LOG(LogTemp, Log, TEXT("[InputPriority] BasicAttack HitResolvedMovementSlowHeldUntilFinish Owner=%s"), *GetNameSafe(GetOwner()));
					}

					if (Result.Breakdown.FinalDamage > 0.f)
					{
						if (UCameraSubsystem* CameraSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UCameraSubsystem>() : nullptr)
						{
							CameraSubsystem->PlayCombatCameraShakeForActor(
								GetOwner(),
								CharacterComp->CharacterDef->BasicAttackCameraShake,
								Result.Breakdown.bCritical);
						}
					}
				}
			}
			break;
		}

	case EPresentedCombatActionType::Skill:
		{
			if (SkillComp.IsValid())
			{
				const FSkillCastResult R = SkillComp->ResolvePreparedSkillCast();
				if (!IsValid(this) || !IsValid(GetOwner()))
				{
					return;
				}

				if (!R.bOk)
				{
					CancelActivePresentation(R.ReasonTag.IsNone() ? "Reject.ResolveFailed" : R.ReasonTag, false);
					return;
				}

				Active.bResolved = R.bOk;
				if (USkillDataAsset* SkillDef = SkillComp->GetSkillDef(Active.ActionId))
				{
					if (UCameraSubsystem* CameraSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UCameraSubsystem>() : nullptr)
					{
						CameraSubsystem->PlayCombatCameraShakeForActor(GetOwner(), SkillDef->CameraShake, false);
					}
				}
			}
			else
			{
				CancelActivePresentation("Reject.NoSkillComponent", false);
				return;
			}
			break;
		}

	case EPresentedCombatActionType::Item:
		{
			if (UCombatItemExecutionSubsystem *Exec = GetWorld()->GetSubsystem<UCombatItemExecutionSubsystem>())
			{
				FCombatItemUseRequest Req;
				Req.User = GetOwner();
				Req.ItemId = Active.ActionId;
				Req.ReasonTag = "Present.Item";
				for (const TWeakObjectPtr<AActor> &W : Active.Targets)
				{
					if (AActor *A = W.Get())Req.Targets.Add(A);
				}

				const FCombatItemUseResult R = Exec->ExecuteUse(Req);
				Active.bResolved = R.bOk;
			}
			break;
		}

	default:
		break;
	}
}

void UCombatPresentationComponent::FinishActivePresentation()
{
	if (!HasActivePresentation())
	{
		return;
	}

	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Presentation,
			"Present.Finish",
			FString::Printf(TEXT("Finish presentation | Type=%d Action=%s Resolved=%s"),
				(int32)Active.Type,
				*Active.ActionId.ToString(),
				Active.bResolved ? TEXT("true") : TEXT("false")),
			GetOwner(),
			Active.Targets.Num() > 0 ? Active.Targets[0].Get() : nullptr,
			FLinearColor(0.8f, 1.f, 0.8f));
	}

	EmitCue(Active.FinishCueTag);

	if (UBattleSessionSubsystem*Battle =GetBattle())
	{
		if (Active.bResolved)
		{			
			float RecoverySec = Battle->GetDefaultActionRecoverySec();

			if (Active.Type == EPresentedCombatActionType::BasicAttack)
			{
				const APawn* OwnerPawn = Cast<APawn>(GetOwner());
				RecoverySec = (OwnerPawn && OwnerPawn->IsPlayerControlled()) ? 0.05f : 0.6f;
			}
			else if (Active.Type == EPresentedCombatActionType::Skill)
			{
				RecoverySec = 0.05f;
			}

			Battle->CompletePresentedAction(GetOwner(),"Present.Finish",RecoverySec);
		}
		else
		{
			Battle->AbortPresentedAction(GetOwner(), "Present.Unresolved");
			if (Active.Type == EPresentedCombatActionType::Skill&&SkillComp.IsValid() && SkillComp->HasPreparedSkillCast())
			{
				SkillComp->CancelPreparedSkillCast(true, "Present.Unresolved");
			}
		}
	}

	if (Active.Type == EPresentedCombatActionType::Skill)
	{
		ClearAutoAttackSuppression();
	}

	OnPresentationFinished.Broadcast(Active.Type, Active.ActionId);
	RestorePresentationMovementSlowIfNeeded();
	RestoreAttackRotationLockIfNeeded();
	ReleaseInputLockForPresentation();
	ClearActiveState();
}

void UCombatPresentationComponent::CancelActivePresentation(FName ReasonTag, bool bRefundPreparedSkill)
{
	if (!HasActivePresentation())
	{
		return;
	}

	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Presentation,
			ReasonTag.IsNone() ? "Present.Cancel" : ReasonTag,
			FString::Printf(TEXT("Cancel presentation | Type=%d Action=%s RefundPreparedSkill=%s"),
				(int32)Active.Type,
				*Active.ActionId.ToString(),
				bRefundPreparedSkill ? TEXT("true") : TEXT("false")),
			GetOwner(),
			Active.Targets.Num() > 0 ? Active.Targets[0].Get() : nullptr,
			FLinearColor(1.f, 0.6f, 0.6f));
	}

	StopActiveMontageIfNeeded(0.08f);
	CancelActiveMotionIfNeeded();

	if (Active.Type == EPresentedCombatActionType::Skill && SkillComp.IsValid() && SkillComp->HasPreparedSkillCast())
	{
		SkillComp->CancelPreparedSkillCast(bRefundPreparedSkill, ReasonTag);
	}

	if (UBattleSessionSubsystem* Battle = GetBattle())
	{
		Battle->AbortPresentedAction(GetOwner(), ReasonTag);
	}

	if (Active.Type == EPresentedCombatActionType::Skill)
	{
		ClearAutoAttackSuppression();
	}

	OnPresentationFinished.Broadcast(Active.Type, Active.ActionId);
	RestorePresentationMovementSlowIfNeeded();
	RestoreAttackRotationLockIfNeeded();
	ReleaseInputLockForPresentation();
	ClearActiveState();
}

bool UCombatPresentationComponent::CancelPlayerBasicAttackForMovement(FName ReasonTag)
{
	if (Active.Type != EPresentedCombatActionType::BasicAttack)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsPlayerControlled())
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[InputPriority] Player movement cancels auto basic attack Owner=%s Resolved=%s"),
		*GetNameSafe(GetOwner()),
		Active.bResolved ? TEXT("true") : TEXT("false"));

	if (Active.bResolved)
	{
		StopActiveMontageIfNeeded(0.08f);
		FinishActivePresentation();
		return true;
	}

	CancelActivePresentation(ReasonTag.IsNone() ? FName("Input.MoveCancelBasicAttack") : ReasonTag, false);
	return true;
}

void UCombatPresentationComponent::ClearActiveState()
{
	Active = FActivePresentationState();
}
