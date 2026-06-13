// Source/JRPGCombat/Private/Combat/Items/ItemUseComponent.cpp
#include "Combat/Items/ItemUseComponent.h"

#include "Combat/Items/ItemDatabaseAsset.h"
#include "Combat/Items/ItemDataAsset.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"

UItemUseComponent::UItemUseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const UItemDataAsset* UItemUseComponent::FindDef(FName ItemId)const
{
	return ItemDB ? ItemDB->FindItem(ItemId) : nullptr;
}

bool UItemUseComponent::PassSessionRule(const UItemDataAsset &Def,FName &OutReason)const
{
	// SessionProvider가 없으면 “체인 중 금지/전투 중 금지” 같은 세밀한 규칙을 체크 못 하므로
	// 안전하게: Chain 금지는 강제, 전투/비전투 금지는 무시(프로토타입 용)
	if (SessionProvider)
	{
		const bool bCombat =SessionProvider->IsCombatActive();
		const bool bChain  =SessionProvider->IsChainSequenceActive();

		if (bChain && !Def.bUsableDuringChainSequence)
		{
			OutReason ="Reject.BlockedDuringChain";
			return false;
		}
		if (bCombat&&!Def.bUsableInCombat)
		{
			OutReason ="Reject.NotUsableInCombat";
			return false;
		}
		if (!bCombat&&!Def.bUsableOutOfCombat)
		{
			OutReason ="Reject.NotUsableOutOfCombat";
			return false;
		}
	}
	return true;
}

bool UItemUseComponent::PassLevelRule(const UItemDataAsset &Def,AActor* User,FName &OutReason)const
{
	if (Def.RequiredLevel <= 0)
		return true;
	
	if (!LevelProvider)
	{
		OutReason = "Reject.LevelProviderMissing";
		return false;
	}

	const int32 Lv =LevelProvider->GetCharacterLevel(User);
	if (Lv < Def.RequiredLevel)
	{
		OutReason ="Reject.LevelTooLow";
		return false;
	}
	return true;
}

bool UItemUseComponent::PassTargetingRule(const UItemDataAsset &Def,AActor* User,AActor* Target,FName &OutReason)const
{
	switch (Def.Targeting)
	{
	case EItemUseTargeting::Self:
		
		if (!Target)
			Target = User;
		
		if (Target!=User)
		{
			OutReason ="Reject.TargetNotSelf";
			return false;
		}
		return true;

	case EItemUseTargeting::AllySingle:
		if (!Target)
		{
			OutReason ="Reject.NoTarget";
			return false;
		}
		return true;

	case EItemUseTargeting::EnemySingle:
		if (!Target)
		{
			OutReason ="Reject.NoTarget";
			return false;
		}
		return true;

	case EItemUseTargeting::PartyAll:
		// PartyProvider 없으면 사용 불가
		if (!PartyProvider)
		{
			OutReason ="Reject.PartyProviderMissing";
			return false;
		}
		return true;

	default:
		OutReason ="Reject.BadTargeting";
		return false;
	}
}

FItemOp UItemUseComponent::TryUseItem(UInventorySubsystem* Inv,FGuid InstanceId, AActor* Target)
{
	if (!Inv)
		return FItemOp::Fail("Reject.NoInventory");
	
	AActor* User = GetOwner();
	
	if (!User) 
		return FItemOp::Fail("Reject.NoUser");

	FItemInstance Inst;
	if (!Inv->TryGetInstance(InstanceId,Inst))
		return FItemOp::Fail("Reject.InstanceNotFound");

	if (Inst.bLocked)
		return FItemOp::Fail("Reject.ItemLocked");

	const UItemDataAsset* Def =FindDef(Inst.ItemId);
	if (!Def)
		return FItemOp::Fail("Reject.ItemNotFound");
	
	if (!Def->IsConsumable())
		return FItemOp::Fail("Reject.NotConsumable");

	FName Reason;
	if (!PassSessionRule(*Def,Reason))				return FItemOp::Fail(Reason);
	if (!PassLevelRule(*Def, User, Reason))			return FItemOp::Fail(Reason);
	if (!PassTargetingRule(*Def, User, Target, Reason))	return FItemOp::Fail(Reason);

	// 1개 소비 -> 실패 시 RestoreInstance 롤백
	const FItemOp Rem = Inv->RemoveItemByInstance(InstanceId,1,"Item.Use");
	if (!Rem.bOk)
		return Rem;

	FItemOp Apply = FItemOp::Ok();

	if (Def->Targeting == EItemUseTargeting::PartyAll)
	{
		TArray<AActor*> Party;
		PartyProvider->GetPartyMembers(Party);
		for (AActor* M : Party)
		{
			if (!M) continue;
			Apply = ApplyConsumableEffects(*Def, User, M,Inst);
			
			if (!Apply.bOk)
				break;
		}
	}
	else
	{
		AActor* FinalTarget = Target ? Target : User;
		Apply = ApplyConsumableEffects(*Def, User, FinalTarget, Inst);
	}

	if (!Apply.bOk)
	{
		Inv->RestoreInstance(Inst, "Item.Use.Rollback");
		return Apply;
	}

	OnItemConsumed.Broadcast(User, Target ? Target : User, Def->ItemId, InstanceId, "Item.Use.Success");
	return FItemOp::Ok();
}

FItemOp UItemUseComponent::ApplyConsumableEffects(const UItemDataAsset &Def,AActor* User,AActor* Target,const FItemInstance &/*UsedInstance*/)
{
	for (const FConsumableEffect &E : Def.UseEffects)
	{
		switch (E.Type)
		{
		case EConsumableEffectType::HealHPFlat:
			ApplyHealHP(User,Target,E.Value,E.SourceTag.IsNone() ? "Item.Heal" : E.SourceTag);
			break;

		case EConsumableEffectType::HealHPPctMax:
			{
				if (UHPComponent* HP = Target ? Target->FindComponentByClass<UHPComponent>() : nullptr)
				{
					const float Amount = HP->GetMaxHP() * FMath::Max(0.f,E.Value);
					ApplyHealHP(User, Target, Amount, E.SourceTag.IsNone() ? "Item.HealPct" : E.SourceTag);
				}
				break;
			}

		case EConsumableEffectType::RestoreAPFlat:
			ApplyRestoreAP(Target, E.Value, E.SourceTag.IsNone() ? "Item.RestoreAP" : E.SourceTag);
			break;

		case EConsumableEffectType::CleanseStatusId:
			ApplyCleanseStatusId(Target, E.StatusId);
			break;

		case EConsumableEffectType::CleanseByTag:
			ApplyCleanseByTag(Target, E.StatusTag);
			break;

		case EConsumableEffectType::ApplyStatusId:
			ApplyApplyStatusId(Target, E);
			break;

		case EConsumableEffectType::GrantSPFlat:
			ApplyGrantSP(User, (int32)E.Value);
			break;

		default:
			break;
		}
	}
	return FItemOp::Ok();
}

void UItemUseComponent::ApplyHealHP(AActor*User,AActor*Target,float Amount,FName/*SourceTag*/)
{
	if (!Target || Amount <= 0.f)
		return;
	
	if (UHPComponent* HP = Target->FindComponentByClass<UHPComponent>())
	{
		HP->Heal(Amount,User,"Item.Heal");
	}
}

void UItemUseComponent::ApplyRestoreAP(AActor* Target,float Amount,FName/*SourceTag*/)
{
	if (!Target || Amount <= 0.f)
		return;
	
	if (UAPComponent* AP = Target->FindComponentByClass<UAPComponent>())
	{
		AP->Restore((int32)FMath::RoundToInt(Amount),"Item.RestoreAP");
	}
}

void UItemUseComponent::ApplyCleanseStatusId(AActor*Target,FName StatusId)
{
	if (!Target || StatusId.IsNone())
		return;

	TArray<UActorComponent*> Comps;
	Target->GetComponents(Comps);
	
	for (UActorComponent* C :Comps)
	{
		if (C && C->GetClass()->ImplementsInterface(UCombatStatusMutator::StaticClass()))
		{
			if (ICombatStatusMutator* M =Cast<ICombatStatusMutator>(C))
			{
				M->RemoveStatusById(StatusId,"Item.Cleanse");
				return;
			}
		}
	}
}

void UItemUseComponent::ApplyCleanseByTag(AActor* Target,const FGameplayTag &Tag)
{
	if (!Target || !Tag.IsValid())
		return;

	TArray<UActorComponent*> Comps;
	Target->GetComponents(Comps);
	
	for (UActorComponent* C : Comps)
	{
		if (C && C->GetClass()->ImplementsInterface(UCombatStatusMutator::StaticClass()))
		{
			if (ICombatStatusMutator* M =Cast<ICombatStatusMutator>(C))
			{
				M->RemoveByTag(Tag,"Item.CleanseTag");
				return;
			}
		}
	}
}

void UItemUseComponent::ApplyApplyStatusId(AActor*Target,const FConsumableEffect &E)
{
	if (!Target || E.StatusId.IsNone())
		return;

	TArray<UActorComponent*> Comps;
	Target->GetComponents(Comps);
	for (UActorComponent* C : Comps)
	{
		if (C && C->GetClass()->ImplementsInterface(UCombatStatusMutator::StaticClass()))
		{
			if (ICombatStatusMutator* M =Cast<ICombatStatusMutator>(C))
			{
				FGameplayTagContainer Tags;
				M->ApplyStatusById(E.StatusId, E.DurationSec, E.Magnitude, E.Stacks, Tags);
				
				return;
			}
		}
	}
}

void UItemUseComponent::ApplyGrantSP(AActor*User,int32 Amount)
{
	if (!User || Amount <= 0)
		return;

	// 월드 내 SP Mutator 구현체 찾기(추후 SynergyPointSubsystem이 구현)
	if (!GetWorld())
		return;
	
	for (TObjectIterator<UObject>It; It; ++It)
	{
		UObject *Obj = *It;
		if (!Obj || Obj->GetWorld()!=GetWorld())
			continue;

		if (Obj->GetClass()->ImplementsInterface(UCombatSPMutator::StaticClass()))
		{
			if (ICombatSPMutator* SP = Cast<ICombatSPMutator>(Obj))
			{
				SP->GrantSP(Amount,"Item.GrantSP", User);
				
				return;
			}
		}
	}
}