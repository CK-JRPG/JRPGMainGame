// Source/JRPGCombat/Private/Combat/Items/CombatItemExecutionSubsystem.cpp
#include "Combat/Items/CombatItemExecutionSubsystem.h"

#include "Combat/Items/CombatItemComponent.h"
#include "Combat/Items/CombatUsableItemDataAsset.h"

#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Status/CombatStatusCleanseInterface.h"

bool UCombatItemExecutionSubsystem::IsAliveCombatant(AActor* Actor) const
{
	if (!Actor) return false;

	if (ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor))
	{
		if (UHPComponent* HP = P->GetHP())
		{
			return !HP->IsDead();
		}
	}
	return false;
}

bool UCombatItemExecutionSubsystem::IsSameTeam(AActor* A, AActor* B) const
{
	if (!A || !B) return false;

	ICombatParticipantInterface* PA = Cast<ICombatParticipantInterface>(A);
	ICombatParticipantInterface* PB = Cast<ICombatParticipantInterface>(B);
	if (!PA || !PB) return false;

	const ECombatTeam TA = PA->GetCombatTeam();
	const ECombatTeam TB = PB->GetCombatTeam();

	if (TA == ECombatTeam::Neutral || TB == ECombatTeam::Neutral) return false;
	return TA == TB;
}

bool UCombatItemExecutionSubsystem::IsEnemyTeam(AActor* A, AActor* B) const
{
	if (!A || !B) return false;

	ICombatParticipantInterface* PA = Cast<ICombatParticipantInterface>(A);
	ICombatParticipantInterface* PB = Cast<ICombatParticipantInterface>(B);
	if (!PA || !PB)returnfalse;

	const ECombatTeam TA = PA->GetCombatTeam();
	const ECombatTeam TB = PB->GetCombatTeam();

	if (TA == ECombatTeam::Neutral || TB == ECombatTeam::Neutral) return false;
	return TA != TB;
}

bool UCombatItemExecutionSubsystem::ValidateTargets(
	AActor* User,
	const UCombatUsableItemDataAsset& ItemDef,
	const TArray<AActor*>& Targets,
	FName& OutReason) const
{
	OutReason = NAME_None;

	if (!User)
	{
		OutReason = "Reject.InvalidUser";
		return false;
	}

	if (!ItemDef.IsValidItem() || !ItemDef.HasAnyRuntimeEffect())
	{
		OutReason = "Reject.InvalidItem";
		return false;
	}

	if (Targets.Num() <= 0)
	{
		OutReason = "Reject.InvalidTarget";
		return false;
	}

	auto RequireAlive = [this](AActor* A) -> bool
	{
		return A && IsAliveCombatant(A);
	};

	switch (ItemDef.TargetType)
	{
	case ESkillTargetType::Self:
		if (Targets.Num() != 1 || Targets[0] != User || !RequireAlive(Targets[0]))
		{
			OutReason = "Reject.InvalidTarget";
			return false;
		}
		break;

	case ESkillTargetType::AllySingle:
		if (Targets.Num() != 1 || !RequireAlive(Targets[0]) || !IsSameTeam(User, Targets[0]))
		{
			OutReason = "Reject.InvalidTarget";
			return false;
		}
		break;

	case ESkillTargetType::AllyAll:
		for (AActor* T : Targets)
		{
			if (!RequireAlive(T) || !IsSameTeam(User, T))
			{
				OutReason = "Reject.InvalidTarget";
				return false;
			}
		}
		break;

	case ESkillTargetType::EnemySingle:
		if (Targets.Num() != 1 || !RequireAlive(Targets[0]) || !IsEnemyTeam(User, Targets[0]))
		{
			OutReason = "Reject.InvalidTarget";
			return false;
		}
		break;

	case ESkillTargetType::EnemyAll:
		for (AActor* T : Targets)
		{
			if (!RequireAlive(T) || !IsEnemyTeam(User, T))
			{
				OutReason = "Reject.InvalidTarget";
				return false;
			}
		}
		break;

	default:
		OutReason = "Reject.InvalidTarget";
		return false;
	}

	return true;
}

bool UCombatItemExecutionSubsystem::WouldAnyEffectApply(
	AActor* User,
	const UCombatUsableItemDataAsset& ItemDef,
	const TArray<AActor*>& Targets) const
{
	for (AActor* T : Targets)
	{
		if (!T)continue;

		UHPComponent* HP = T->FindComponentByClass<UHPComponent>();
		UAPComponent* AP = T->FindComponentByClass<UAPComponent>();
		USPComponent* SP = T->FindComponentByClass<USPComponent>();

		if (HP && ItemDef.HealHP > 0.f && HP->GetHP() < HP->GetMaxHP())
			return true;

		if (AP && ItemDef.RestoreAP > 0 && AP->GetAP() < AP->GetMaxAP())
			return true;

		if (SP && ItemDef.GrantSP > 0)
			return true;

		if (HP && ItemDef.FlatDamage > 0.f && !HP->IsDead())
			return true;

		if (ItemDef.FlatGroggyDamage > 0.f)
			return true;

		if (ItemDef.FlatThreatToUserOnTarget != 0.f)
			return true;

		if (ItemDef.ApplyStatus != nullptr)
			return true;

		if (ItemDef.DispelAnyTags.Num() > 0)
		{
			if (UActorComponent* Comp = T->FindComponentByClass<UStatusEffectComponent>())
			{
				if (ICombatStatusCleanseInterface* Cleanse = Cast<ICombatStatusCleanseInterface>(Comp))
				{
					if (Cleanse->CountStatusesByAnyTags(ItemDef.DispelAnyTags) > 0)
						return true;
				}
			}
		}
	}

	return false;
}

FCombatItemUseResult UCombatItemExecutionSubsystem::ExecuteUse(const FCombatItemUseRequest& Request)
{
	AActor* User = Request.User.Get();
	if (!User)return FCombatItemUseResult::Fail("Reject.InvalidUser");
	if (!IsAliveCombatant(User))return FCombatItemUseResult::Fail("Reject.UserDead");

	UCombatItemComponent* ItemComp = User->FindComponentByClass<UCombatItemComponent>();
	if (!ItemComp)return FCombatItemUseResult::Fail("Reject.NoItemComponent");

	UCombatUsableItemDataAsset* ItemDef = ItemComp->FindItemDef(Request.ItemId);
	if (!ItemDef)return FCombatItemUseResult::Fail("Reject.ItemNotFound");
	if (!ItemComp->HasItem(ItemDef->ItemId, 1))return FCombatItemUseResult::Fail("Reject.OutOfItem");

	TArray<AActor*> Targets;
	for (const TWeakObjectPtr<AActor>& W : Request.Targets)
	{
		if (AActor* A = W.Get())Targets.Add(A);
	}

	FName ValidateReason = NAME_None;
	if (!ValidateTargets(User, *ItemDef, Targets, ValidateReason))
	{
		return FCombatItemUseResult::Fail(ValidateReason);
	}

	if (!WouldAnyEffectApply(User, *ItemDef, Targets))
	{
		return FCombatItemUseResult::Fail("Reject.NoEffect");
	}

	if (ItemDef->bConsumeOnUse)
	{
		if (!ItemComp->ConsumeItem(ItemDef->ItemId, 1, Request.ReasonTag))
		{
			return FCombatItemUseResult::Fail("Reject.OutOfItem");
		}
	}

	FCombatItemUseResult Out = FCombatItemUseResult::Ok();
	Out.User = User;
	Out.ItemId = ItemDef->ItemId;
	Out.ReasonTag = Request.ReasonTag;
	for (AActor* T : Targets)
	{
		Out.Targets.Add(T);
	}

	for (AActor* T : Targets)
	{
		if (!T)continue;

		UHPComponent* HP = T->FindComponentByClass<UHPComponent>();
		UAPComponent* AP = T->FindComponentByClass<UAPComponent>();
		USPComponent* SP = T->FindComponentByClass<USPComponent>();
		UGroggyComponent* Groggy = T->FindComponentByClass<UGroggyComponent>();
		UThreatComponent* Threat = T->FindComponentByClass<UThreatComponent>();
		UStatusEffectComponent* Status = T->FindComponentByClass<UStatusEffectComponent>();

		if (HP && ItemDef->HealHP > 0.f)
		{
			const float Before = HP->GetHP();
			HP->Heal(ItemDef->HealHP, User, ItemDef->ItemId);
			const float After = HP->GetHP();
			Out.Breakdown.TotalHealedHP += FMath::Max(0.f, After - Before);
		}

		if (AP && ItemDef->RestoreAP > 0)
		{
			const int32 Before = AP->GetAP();
			AP->Restore(ItemDef->RestoreAP, ItemDef->ItemId);
			const int32 After = AP->GetAP();
			Out.Breakdown.TotalRestoredAP += FMath::Max(0, After - Before);
		}

		if (SP && ItemDef->GrantSP > 0)
		{
			const int32 Before = SP->GetSP();
			SP->AddSP(ItemDef->GrantSP, ItemDef->ItemId);
			const int32 After = SP->GetSP();
			Out.Breakdown.TotalGrantedSP += FMath::Max(0, After - Before);
		}

		if (HP && ItemDef->FlatDamage > 0.f)
		{
			const float Before = HP->GetHP();
			HP->ApplyDamage(ItemDef->FlatDamage, User, ItemDef->ItemId);
			const float After = HP->GetHP();
			Out.Breakdown.TotalDealtDamage += FMath::Max(0.f, Before - After);
		}

		if (Groggy && ItemDef->FlatGroggyDamage > 0.f)
		{
			Groggy->AddGroggyDamage(ItemDef->FlatGroggyDamage, User, ItemDef->ItemId);
			Out.Breakdown.TotalGroggyDamage += ItemDef->FlatGroggyDamage;
		}

		if (Threat && !FMath::IsNearlyZero(ItemDef->FlatThreatToUserOnTarget))
		{
			Threat->AddThreat(User, ItemDef->FlatThreatToUserOnTarget, ItemDef->ItemId);
			Out.Breakdown.TotalThreatAdded += ItemDef->FlatThreatToUserOnTarget;
		}

		if (Status && ItemDef->ApplyStatus)
		{
			if (FMath::FRand() <= FMath::Clamp(ItemDef->StatusChance, 0.f, 1.f))
			{
				Status->ApplyStatus(ItemDef->ApplyStatus, User, ItemDef->StatusStacks, ItemDef->ItemId);
				Out.Breakdown.StatusAppliedCount += 1;
			}
		}

		if (Status && ItemDef->DispelAnyTags.Num() > 0)
		{
			if (ICombatStatusCleanseInterface* Cleanse = Cast<ICombatStatusCleanseInterface>(Status))
			{
				const int32 Removed = Cleanse->RemoveStatusesByAnyTags(
					ItemDef->DispelAnyTags,
					ItemDef->DispelRemoveCount,
					User,
					ItemDef->ItemId);

				Out.Breakdown.StatusRemovedCount += Removed;
			}
		}
	}

	OnCombatItemUsed.Broadcast(Out);
	return Out;
}
