#include "Combat/Presentation/CombatPresentationComponent.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Items/CombatItemExecutionSubsystem.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Skills/SkillDataAsset.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"

#include "Combat/Motion/CombatMotionComponent.h"

#include "GameFramework/Character.h"

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
	if (!GetOwner() || HasActivePresentation()) return;
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
	
	FTacticalReservation R;
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

	FCombatMotionRequest Req = CharacterComp->CharacterDef->BasicAttackMotion;
	Req.Instigator = GetOwner();
	if (Active.Targets.Num() > 0) Req.Target = Active.Targets[0];
	Req.OwnerTag = "BasicAttack";

	if (Req.ExecMode == ECombatMotionExecMode::RootMotion && Req.RootMontage == nullptr)
	{
		Req.RootMontage = CharacterComp->CharacterDef->BasicAttackMontage;
		Req.bMontageDrivenExternally = true;
	}

	const FCombatMotionResponse Resp = Motion->RequestCombatMotion(Req);
	if (Resp.Result == ECombatMotionResult::Accepted || Resp.Result == ECombatMotionResult::ReplacedExisting)
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

	FCombatMotionRequest Req = SkillDef->SkillMotion;
	Req.Instigator = GetOwner();
	if (Active.Targets.Num() > 0) Req.Target = Active.Targets[0];
	Req.OwnerTag = SkillDef->SkillId;

	if (Req.ExecMode == ECombatMotionExecMode::RootMotion && Req.RootMontage == nullptr)
	{
		Req.RootMontage = SkillDef->CastMontage;
		Req.bMontageDrivenExternally = true;
	}

	const FCombatMotionResponse Resp = Motion->RequestCombatMotion(Req);
	if (Resp.Result == ECombatMotionResult::Accepted || Resp.Result == ECombatMotionResult::ReplacedExisting)
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

void UCombatPresentationComponent::PlayActiveMontageOrResolve()
{
	EmitCue(Active.StartCueTag);

	if (Active.ResolveTiming == ECombatResolveTiming::Immediate || !Active.Montage)
	{
		ResolveActivePresentation();
		FinishActivePresentation();
		return;
	}

	if (ACharacter *C = Cast<ACharacter>(GetOwner()))
	{
		C->PlayAnimMontage(Active.Montage);
	}
	else
	{
		ResolveActivePresentation();
		FinishActivePresentation();
	}
}

FCombatActionResult UCombatPresentationComponent::TryPresentBasicAttack(AActor *Target)
{
	UBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle)
		return FCombatActionResult::Fail("Reject.NoBattleSession");
	
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
	
	OnPresentationStarted.Broadcast(Active.Type, Active.ActionId);
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
	
	OnPresentationStarted.Broadcast(Active.Type, Active.ActionId);
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
	
	if (!HasActivePresentation())
		return;
	if (Active.bResolved)
		return;

	UBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle || !Battle->CanActorResolvePresentedAction(GetOwner()))
		return;

	EmitCue(Active.HitCueTag);

	switch (Active.Type)
	{
	case EPresentedCombatActionType::BasicAttack:
		{
			if (!CharacterComp.IsValid()||!CharacterComp->CharacterDef)break;
			if (Active.Targets.Num()<=0)break;

			AActor*Target =Active.Targets[0].Get();
			if (!Target)break;

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
				Basic->ExecuteBasicAttack(Req);
			}
			Active.bResolved = true;
			break;
		}

	case EPresentedCombatActionType::Skill:
		{
			if (SkillComp.IsValid())
			{
				const FSkillCastResult R = SkillComp->ResolvePreparedSkillCast();
				Active.bResolved = R.bOk;
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
	
	if (!HasActivePresentation())
		return;

	EmitCue(Active.FinishCueTag);

	if (UBattleSessionSubsystem*Battle =GetBattle())
	{
		if (Active.bResolved)
		{			
			float RecoverySec = Battle->GetDefaultActionRecoverySec();

			if (Active.Type == EPresentedCombatActionType::BasicAttack)
			{
				if (CharacterComp.IsValid()&&CharacterComp->CharacterDef && CharacterComp->CharacterDef->BasicAttackMontage)
				{
					RecoverySec = CharacterComp->CharacterDef->BasicAttackMontage->GetPlayLength();
				}
			}
			else if (Active.Type == EPresentedCombatActionType::Skill)
			{
				if (SkillComp.IsValid())
				{
					if (USkillDataAsset* Def = SkillComp->GetSkillDef(Active.ActionId))
					{
						if (Def->CastMontage)
						{
							RecoverySec = Def->CastMontage->GetPlayLength();
						}
					}
				}
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

	OnPresentationFinished.Broadcast(Active.Type, Active.ActionId);
	ClearActiveState();
}

void UCombatPresentationComponent::CancelActivePresentation(FName ReasonTag, bool bRefundPreparedSkill)
{
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
	
	CancelActiveMotionIfNeeded();
	
	if (!HasActivePresentation())
		return;

	if (Active.Type == EPresentedCombatActionType::Skill && SkillComp.IsValid() && SkillComp->HasPreparedSkillCast())
	{
		SkillComp->CancelPreparedSkillCast(bRefundPreparedSkill, ReasonTag);
	}

	if (UBattleSessionSubsystem *Battle = GetBattle())
	{
		Battle->AbortPresentedAction(GetOwner(),ReasonTag);
	}

	OnPresentationFinished.Broadcast(Active.Type, Active.ActionId);
	ClearActiveState();
}

void UCombatPresentationComponent::ClearActiveState()
{
	Active = FActivePresentationState();
}
