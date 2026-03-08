// Source/JRPGCombat/Private/Combat/Items/CombatItemExecutionSubsystem.cpp
#include "Combat/Items/CombatItemExecutionSubsystem.h"

#include "Combat/Items/CombatItemComponent.h"
#include "Combat/Items/CombatUsableItemDataAsset.h"

#include "Combat/Characters/CombatParticipantInterface.h"

#include "Combat/Stats/CombatHPComponent.h"
#include "Combat/Stats/CombatAPComponent.h"

#include "Combat/SP/SPComponent.h"
#include "Combat/SP/CombatSynergyPointSubsystem.h"

#include "Combat/Groggy/CombatGroggyComponent.h"
#include "Combat/Threat/CombatThreatComponent.h"

#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Status/CombatStatusCleanseInterface.h"

#include "Combat/Debug/CombatDebugSubsystem.h"

bool UCombatItemExecutionSubsystem::IsAliveCombatant(AActor* Actor) const
{
	if (!Actor) return false;

	if (ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor))
	{
		if (UCombatHPComponent* HP = P->GetHP())
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
	if (!A || !B) 
		return false;

	ICombatParticipantInterface* PA = Cast<ICombatParticipantInterface>(A);
	ICombatParticipantInterface* PB = Cast<ICombatParticipantInterface>(B);
	
	if (!PA || !PB) 
		return false;

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

		UCombatHPComponent* HP = T->FindComponentByClass<UCombatHPComponent>();
		UCombatAPComponent* AP = T->FindComponentByClass<UCombatAPComponent>();
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
	if (!User)
	{
		// 실패 디버그 1 (Reject.InvalidUser)
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			Debug->AddLog(
				ECombatDebugCategory::Item,
				Request.ItemId,
				TEXT("Item use failed | Reject.InvalidUser"),
				User,
				Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
				FLinearColor(1.f, 0.5f, 0.5f));
		}
		
		return FCombatItemUseResult::Fail("Reject.InvalidUser");
	}
	
	if (!IsAliveCombatant(User))
	{
		// 실패 디버그 2 (Reject.UserDead)
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			Debug->AddLog(
				ECombatDebugCategory::Item,
				Request.ItemId,
				TEXT("Item use failed | Reject.UserDead"),
				User,
				Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
				FLinearColor(1.f, 0.5f, 0.5f));
		}
		
		return FCombatItemUseResult::Fail("Reject.UserDead");
	}

	UCombatItemComponent* ItemComp = User->FindComponentByClass<UCombatItemComponent>();
	UCombatSynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<UCombatSynergyPointSubsystem>() : nullptr;
	
	if (!ItemComp)
	{
		// 실패 디버그 3 (Reject.NoItemComponent)
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			Debug->AddLog(
				ECombatDebugCategory::Item,
				Request.ItemId,
				TEXT("Item use failed | Reject.NoItemComponent"),
				User,
				Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
				FLinearColor(1.f, 0.5f, 0.5f));
		}
		
		return FCombatItemUseResult::Fail("Reject.NoItemComponent");
	}

	UCombatUsableItemDataAsset* ItemDef = ItemComp->FindItemDef(Request.ItemId);
	if (!ItemDef)
	{
		// 실패 디버그 4 (Reject.ItemNotFound)
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			Debug->AddLog(
				ECombatDebugCategory::Item,
				Request.ItemId,
				TEXT("Item use failed | Reject.ItemNotFound"),
				User,
				Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
				FLinearColor(1.f, 0.5f, 0.5f));
		}
		
		return FCombatItemUseResult::Fail("Reject.ItemNotFound");
	}
	
	if (!ItemComp->HasItem(ItemDef->ItemId, 1))
	{
		// 실패 디버그 5 (Reject.OutOfItem)
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			Debug->AddLog(
				ECombatDebugCategory::Item,
				Request.ItemId,
				TEXT("Item use failed | Reject.OutOfItem"),
				User,
				Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
				FLinearColor(1.f, 0.5f, 0.5f));
		}
		
		return FCombatItemUseResult::Fail("Reject.OutOfItem");
	}

	TArray<AActor*> Targets;
	for (const TWeakObjectPtr<AActor>& W : Request.Targets)
	{
		if (AActor* A = W.Get())Targets.Add(A);
	}

	FName ValidateReason = NAME_None;
	if (!ValidateTargets(User, *ItemDef, Targets, ValidateReason))
	{
		// 실패 디버그 6 (ValidateReason)
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			// 동적으로 변하는 실패 사유이므로 Printf 사용했습니다.
			Debug->AddLog(
				ECombatDebugCategory::Item,
				Request.ItemId,
				FString::Printf(TEXT("Item use failed | ValidateReason: %s"), *ValidateReason.ToString()),
				User,
				Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
				FLinearColor(1.f, 0.5f, 0.5f));
		}
		
		return FCombatItemUseResult::Fail(ValidateReason);
	}

	if (!WouldAnyEffectApply(User, *ItemDef, Targets))
	{
		// 실패 디버그 7 (Reject.NoEffect)
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			Debug->AddLog(
				ECombatDebugCategory::Item,
				Request.ItemId,
				TEXT("Item use failed | Reject.NoEffect"),
				User,
				Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
				FLinearColor(1.f, 0.5f, 0.5f));
		}
		
		return FCombatItemUseResult::Fail("Reject.NoEffect");
	}

	if (ItemDef->bConsumeOnUse)
	{
		if (!ItemComp->ConsumeItem(ItemDef->ItemId, 1, Request.ReasonTag))
		{
			// 실패 디버그 8 (Reject.OutOfItem (On Consume))
			if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
			{
				Debug->AddLog(
					ECombatDebugCategory::Item,
					Request.ItemId,
					TEXT("Item use failed | Reject.OutOfItem (On Consume)"),
					User,
					Request.Targets.Num() > 0 ? Request.Targets[0].Get() : nullptr,
					FLinearColor(1.f, 0.5f, 0.5f));
			}
			
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

		UCombatHPComponent* HP = T->FindComponentByClass<UCombatHPComponent>();
		UCombatAPComponent* AP = T->FindComponentByClass<UCombatAPComponent>();
		USPComponent* SP = T->FindComponentByClass<USPComponent>();
		UGroggyComponent* Groggy = T->FindComponentByClass<UGroggyComponent>();
		UCombatThreatComponent* Threat = T->FindComponentByClass<UCombatThreatComponent>();
		UStatusEffectComponent* Status = T->FindComponentByClass<UStatusEffectComponent>();

		if (HP&&ItemDef->HealHP > 0.f)
		{
			const float Before = HP->GetHP();
			const float BeforeRatio = HP->GetMaxHP() > 0.f ? (Before / HP->GetMaxHP()) : 1.f;

			HP->Heal(ItemDef->HealHP, User, ItemDef->ItemId);

			const float After = HP-> GetHP();
			const float Delta = FMath::Max(0.f,After-Before);

			Out.Breakdown.TotalHealedHP += Delta;

			if (SP && Delta > 0.f)
			{
				SP->ReportHeal(User,T,Delta,BeforeRatio,false,ItemDef->ItemId);
			}
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

		if (HP&&ItemDef->FlatDamage>0.f)
		{
			const float Before = HP->GetHP();
			HP->ApplyDamage(ItemDef->FlatDamage, User, ItemDef->ItemId);
			const float After = HP->GetHP();
			const float Delta = FMath::Max(0.f, Before - After);

			Out.Breakdown.TotalDealtDamage+=Delta;

			if (SP && Delta > 0.f)
			{
				SP->ReportDamage(User, T, Delta, false, ItemDef->ItemId);
			}
		}

		if (Groggy && ItemDef->FlatGroggyDamage > 0.f)
		{
			Groggy->AddGroggyDamage(ItemDef->FlatGroggyDamage, User, ItemDef->ItemId);
			Out.Breakdown.TotalGroggyDamage += ItemDef->FlatGroggyDamage;

			if (SP)
			{
				SP->ReportBreak(User, T, ItemDef->FlatGroggyDamage, false, false, ItemDef->ItemId);
			}
		}

		if (Threat && !FMath::IsNearlyZero(ItemDef->FlatThreatToUserOnTarget))
		{
			Threat->AddThreat(User, ItemDef->FlatThreatToUserOnTarget, ItemDef->ItemId);
			Out.Breakdown.TotalThreatAdded += ItemDef->FlatThreatToUserOnTarget;
		}

		if (Status && ItemDef->ApplyStatus)
		{
			if (FMath::FRand() <= FMath::Clamp(ItemDef->StatusChance,0.f,1.f))
			{
				Status->ApplyStatus(ItemDef->ApplyStatus, User, ItemDef->StatusStacks, ItemDef->ItemId);
				Out.Breakdown.StatusAppliedCount += 1;

				if (SP)
				{
					ICombatParticipantInterface* PU = Cast<ICombatParticipantInterface>(User);
					ICombatParticipantInterface* PT = Cast<ICombatParticipantInterface>(T);

					const bool bEnemy =
						PU && PT &&
						PU->GetCombatTeam() != ECombatTeam::Neutral &&
						PT->GetCombatTeam() != ECombatTeam::Neutral &&
						PU->GetCombatTeam() != PT->GetCombatTeam();

					if (bEnemy)
					{
						SP->ReportDebuff(User, T, ItemDef->ApplyStatus->StatusId, false, ItemDef->ItemId);
					}
					else
					{
						SP->ReportBuff(User, T, ItemDef->ApplyStatus->StatusId, false, ItemDef->ItemId);
					}
				}
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

				if (SP && Removed > 0)
				{
					bool bCriticalCC = false;
					for (const FGameplayTag &Tag : ItemDef->DispelAnyTags)
					{
						if (Tag.ToString().Contains(TEXT("CC")))
						{
							bCriticalCC = true;
							break;
						}
					}

					SP->ReportCleanse(User, T, Removed, bCriticalCC, false, ItemDef->ItemId);
				}
			}
		}
	}

	OnCombatItemUsed.Broadcast(Out);
	
	// 성공 디버그
	if (UCombatDebugSubsystem*Debug =GetWorld() ?GetWorld()->GetSubsystem<UCombatDebugSubsystem>() :nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Item,
			ItemDef->ItemId,
			FString::Printf(
				TEXT("Item used | Heal=%.0f AP=%d SP=%d Damage=%.0f StatusApplied=%d StatusRemoved=%d"),
					Out.Breakdown.TotalHealedHP,
					Out.Breakdown.TotalRestoredAP,
					Out.Breakdown.TotalGrantedSP,
					Out.Breakdown.TotalDealtDamage,
					Out.Breakdown.StatusAppliedCount,
					Out.Breakdown.StatusRemovedCount),
			User,
			Targets.Num()>0 ?Targets[0] :nullptr,
			FLinearColor(1.f,1.f,0.6f));
	}
	
	return Out;
}
