#pragma once

#include "CoreMinimal.h"
#include "JRPGCoreApiTypes.h"
#include "Components/ActorComponent.h"
#include "Animation/AnimMontage.h"

#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "Combat/Skills/SkillTypes.h"
#include "Combat/Items/CombatItemTypes.h"
#include "Combat/Motion/CombatMotionTypes.h"

#include "Combat/Debug/CombatDebugSubsystem.h"
#include "Combat/Skills/SkillDataAsset.h"

#include "CombatPresentationComponent.generated.h"

class USkillComponent;
class UCombatCharacterComponent;
class UTacticalModeSubsystem;
class UBattleSessionSubsystem;
class UCharacterMovementComponent;
class UAnimMontage;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatPresentationComponent();

	FOnCombatCueEvent OnCombatCueEvent;
	FOnPresentationStarted OnPresentationStarted;
	FOnPresentationFinished OnPresentationFinished;

	bool HasActivePresentation()const { return Active.Type!= EPresentedCombatActionType::None; }

	FCombatActionResult TryPresentBasicAttack(AActor*Target);
	FSkillCastResult TryPresentSkill(FName SkillId, const TArray<AActor*> &Targets,bool bFromTacticalReservation = false);
	FCombatItemUseResult TryPresentItem(FName ItemId, const TArray<AActor*> &Targets);

	void ResolveActivePresentation();
	void FinishActivePresentation();


	UFUNCTION()
	void HandleActiveMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void CancelActivePresentation(FName ReasonTag,bool bRefundPreparedSkill);
	bool CancelPlayerBasicAttackForMovement(FName ReasonTag);

	void EmitCue(FName CueTag);
	void SetAutoAttackSuppressedFor(float DurationSec);
	void ClearAutoAttackSuppression();
	bool IsAutoAttackSuppressed() const;
	float GetMinBasicAttackStartInterval() const;
	float GetRemainingBasicAttackStartCooldown() const;

protected:
	virtual void BeginPlay()override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
	struct FActivePresentationState
	{
		EPresentedCombatActionType Type = EPresentedCombatActionType::None;
		FName ActionId = NAME_None;
		TArray<TWeakObjectPtr<AActor>> Targets;
		bool bResolved = false;
		bool bFromTacticalReservation = false;

		ECombatResolveTiming ResolveTiming = ECombatResolveTiming::Immediate;
		TObjectPtr<UAnimMontage> Montage = nullptr;

		FName StartCueTag = NAME_None;
		FName HitCueTag = NAME_None;
		FName FinishCueTag = NAME_None;
		
		FJRPGCombatMotionHandle MotionHandle;
		bool bHasMotion = false;
		double StartedAtRealSec = 0.0;
		double AutoResolveAtRealSec = 0.0;
		double AutoFinishAtRealSec = 0.0;
		
		FJRPGHandle InputLockHandle;
		bool bHasInputLock = false;
		bool bHasMovementSlow = false;
		float SavedMaxWalkSpeed = 0.f;

		bool bHasRotationLock = false;
		bool bSavedOrientRotationToMovement = true;
		bool bSavedUseControllerDesiredRotation = false;
		bool bSavedUseControllerRotationYaw = false;
		double FaceTargetUntilRealSec = 0.0;
	};
	double AutoAttackSuppressedUntilRealSec = 0.0;
	double LastBasicAttackStartWorldTime = -1000.0;

	FActivePresentationState Active;

	TWeakObjectPtr<USkillComponent> SkillComp;
	TWeakObjectPtr<UCombatCharacterComponent> CharacterComp;

	UBattleSessionSubsystem* GetBattle()const;
	UTacticalModeSubsystem* GetTactical()const;

	void PlayActiveMontageOrResolve();
	void ClearActiveState();

	void TryConsumeTacticalReservation();
	
	bool TryStartMotionForBasicAttack();
	bool TryStartMotionForSkill(USkillDataAsset* SkillDef);
	void CancelActiveMotionIfNeeded();
	void StopActiveMontageIfNeeded(float BlendOutTime);
	
	void AcquireInputLockForPresentation();
	void ReleaseInputLockForPresentation();
	void ApplyPresentationMovementSlowIfNeeded();
	void RestorePresentationMovementSlowIfNeeded();
	void ApplyAttackRotationLockIfNeeded();
	void RestoreAttackRotationLockIfNeeded();
	void FaceActiveTarget();
	void StopPathFollowingForPresentation(FName ReasonTag);
	void ConfigureAutoPresentationTiming(bool bNoPlayableMontage);
};
