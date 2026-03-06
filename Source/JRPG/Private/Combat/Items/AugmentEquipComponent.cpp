// Source/JRPGCombat/Private/Combat/Items/AugmentEquipComponent.cpp
#include "Combat/Items/AugmentEquipComponent.h"
#include "Combat/Items/ItemDatabaseAsset.h"
#include "Combat/Items/ItemDataAsset.h"
#include "Combat/Items/ItemCapSettingsDataAsset.h"

UAugmentEquipComponent::UAugmentEquipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Slots.Add(EAugmentEquipSlot::AugmentSlot1, FAugmentSlotState());
	Slots.Add(EAugmentEquipSlot::AugmentSlot2, FAugmentSlotState());
	Slots.Add(EAugmentEquipSlot::AugmentSlot3, FAugmentSlotState());
}

bool UAugmentEquipComponent::IsSlotOccupied(EAugmentEquipSlot Slot) const
{
	if (const FAugmentSlotState* S = Slots.Find(Slot)) return S->bOccupied;
	return false;
}

FAugmentSlotState UAugmentEquipComponent::GetSlotState(EAugmentEquipSlot Slot) const
{
	if (const FAugmentSlotState* S = Slots.Find(Slot)) return *S;
	return FAugmentSlotState();
}

const UItemDataAsset* UAugmentEquipComponent::FindDef(FName ItemId) const
{
	return ItemDB ? ItemDB->FindItem(ItemId) : nullptr;
}

bool UAugmentEquipComponent::HasUniqueGroupEquipped(FName UniqueGroup) const
{
	if (UniqueGroup.IsNone()) return false;

	for (const auto& KV : Slots)
	{
		const FAugmentSlotState& S = KV.Value;
		if (!S.bOccupied) continue;

		const UItemDataAsset* Def = FindDef(S.StoredInstance.ItemId);
		if (Def && Def->UniqueEquipGroup == UniqueGroup)
			return true;
	}
	return false;
}

FItemOp UAugmentEquipComponent::ValidateEquip(const UItemDataAsset* Def, EAugmentEquipSlot Slot) const
{
	if (!Def) return FItemOp::Fail("Reject.ItemNotFound");
	if (!Def->IsAugment()) return FItemOp::Fail("Reject.ItemTypeNotAugment");
	if (!Def->IsSlotAllowed(Slot)) return FItemOp::Fail("Reject.SlotInvalid");

	if (Def->RequiredLevel > 0)
	{
		if (!LevelProvider) return FItemOp::Fail("Reject.LevelProviderMissing");
		const int32 CurLv = LevelProvider->GetCharacterLevel(GetOwner());
		if (CurLv < Def->RequiredLevel) return FItemOp::Fail("Reject.LevelTooLow");
	}

	if (!Def->IsRoleAllowed(Role))
		return FItemOp::Fail("Reject.RoleNotAllowed");

	if (!Def->UniqueEquipGroup.IsNone() && HasUniqueGroupEquipped(Def->UniqueEquipGroup))
		return FItemOp::Fail("Reject.UniqueConflict");

	return FItemOp::Ok();
}

FItemOp UAugmentEquipComponent::TryEquipFromInventory(UInventorySubsystem* Inventory, FGuid InstanceId,
                                                      EAugmentEquipSlot Slot)
{
	if (!Inventory) return FItemOp::Fail("Reject.NoInventory");

	FItemInstance Inst;
	if (!Inventory->TryGetInstance(InstanceId, Inst))
	{
		OnAugmentEquipRejected.Broadcast(CharacterId, NAME_None, "Reject.InstanceNotFound");
		return FItemOp::Fail("Reject.InstanceNotFound");
	}

	const UItemDataAsset* Def = FindDef(Inst.ItemId);
	const FItemOp V = ValidateEquip(Def, Slot);
	if (!V.bOk)
	{
		OnAugmentEquipRejected.Broadcast(CharacterId, Inst.ItemId, V.ReasonTag);
		return V;
	}

	if (IsSlotOccupied(Slot))
	{
		const FItemOp Un = TryUnequipToInventory(Inventory, Slot);
		if (!Un.bOk) return Un;
	}

	const FItemOp Rem = Inventory->RemoveItemByInstance(InstanceId, 1, "Equip.ToSlot");
	if (!Rem.bOk)
	{
		OnAugmentEquipRejected.Broadcast(CharacterId, Inst.ItemId, Rem.ReasonTag);
		return Rem;
	}

	FAugmentSlotState& S = Slots.FindChecked(Slot);
	const FName OldItemId = S.bOccupied ? S.StoredInstance.ItemId : NAME_None;

	S.bOccupied = true;
	S.StoredInstance = Inst;

	Inventory->NotifyEquipped(Inst.InstanceId);

	RebuildModifierSet();
	OnModifierSetChanged.Broadcast(CharacterId);
	OnAugmentEquipped.Broadcast(CharacterId, Slot, OldItemId, Inst.ItemId, "Equip.Success");
	return FItemOp::Ok();
}

FItemOp UAugmentEquipComponent::TryUnequipToInventory(UInventorySubsystem* Inventory, EAugmentEquipSlot Slot)
{
	if (!Inventory) return FItemOp::Fail("Reject.NoInventory");

	FAugmentSlotState* S = Slots.Find(Slot);
	if (!S || !S->bOccupied) return FItemOp::Fail("Reject.SlotEmpty");

	const FItemOp Restore = Inventory->RestoreInstance(S->StoredInstance, "Unequip.RestoreInstance");
	if (!Restore.bOk) return Restore;

	Inventory->NotifyUnequipped(S->StoredInstance.InstanceId);

	const FName OldItemId = S->StoredInstance.ItemId;

	S->bOccupied = false;
	S->StoredInstance = FItemInstance();

	RebuildModifierSet();
	OnModifierSetChanged.Broadcast(CharacterId);
	OnAugmentEquipped.Broadcast(CharacterId, Slot, OldItemId, NAME_None, "Unequip.Success");
	return FItemOp::Ok();
}

float UAugmentEquipComponent::ApplyCapPct(FName CapGroup, float PctSum) const
{
	if (!CapSettings) return PctSum;
	const float Cap = CapSettings->GetCapPct(CapGroup, 9999.f);
	return FMath::Clamp(PctSum, -Cap, Cap);
}

void UAugmentEquipComponent::ApplyEffectWithCaps(const UItemDataAsset* Def, const FAugmentEffect& E, float RoleEff)
{
	const float V = E.Value * RoleEff;

	auto AddPct = [&](float& PctSum)
	{
		PctSum += V;
		PctSum = ApplyCapPct(E.CapGroup, PctSum);
	};

	auto AddSkillPct = [&](TMap<FName, float>& Map)
	{
		if (E.TargetSkillTag.IsNone()) return;
		float& P = Map.FindOrAdd(E.TargetSkillTag);
		P += V;
		P = ApplyCapPct(E.CapGroup, P);
	};

	switch (E.EffectType)
	{
	case EAugmentEffectType::AttackFlat: CachedMods.Attack.Flat += V;
		break;
	case EAugmentEffectType::AttackPct: AddPct(CachedMods.Attack.Pct);
		break;
	case EAugmentEffectType::DefenseFlat: CachedMods.Defense.Flat += V;
		break;
	case EAugmentEffectType::DefensePct: AddPct(CachedMods.Defense.Pct);
		break;
	case EAugmentEffectType::HPFlat: CachedMods.HP.Flat += V;
		break;
	case EAugmentEffectType::HPPct: AddPct(CachedMods.HP.Pct);
		break;
	case EAugmentEffectType::BreakPowerFlat: CachedMods.BreakPower.Flat += V;
		break;
	case EAugmentEffectType::BreakPowerPct: AddPct(CachedMods.BreakPower.Pct);
		break;
	case EAugmentEffectType::HealingPowerFlat: CachedMods.HealingPower.Flat += V;
		break;
	case EAugmentEffectType::HealingPowerPct: AddPct(CachedMods.HealingPower.Pct);
		break;
	case EAugmentEffectType::ThreatModPct: AddPct(CachedMods.ThreatModPct);
		break;

	case EAugmentEffectType::AoERadiusPct: AddSkillPct(CachedMods.AoERadius.PctBySkillTag);
		break;
	case EAugmentEffectType::BuffDurationPct: AddSkillPct(CachedMods.BuffDuration.PctBySkillTag);
		break;
	case EAugmentEffectType::DebuffPotencyPct: AddSkillPct(CachedMods.DebuffPotency.PctBySkillTag);
		break;
	case EAugmentEffectType::BreakBuildUpPct: AddSkillPct(CachedMods.BreakBuildUp.PctBySkillTag);
		break;
	default: break;
	}
}

void UAugmentEquipComponent::RebuildModifierSet()
{
	CachedMods = FAugmentModifierSet();

	for (const auto& KV : Slots)
	{
		const FAugmentSlotState& S = KV.Value;
		if (!S.bOccupied) continue;

		const UItemDataAsset* Def = FindDef(S.StoredInstance.ItemId);
		if (!Def) continue;

		const float RoleEff = Def->RoleEfficiency.Get(Role);
		for (const FAugmentEffect& E : Def->EffectList)
			ApplyEffectWithCaps(Def, E, RoleEff);
	}
}

void UAugmentEquipComponent::ExportSaveData(FAugmentEquipSaveData& Out) const
{
	Out = FAugmentEquipSaveData();
	Out.CharacterId = CharacterId;

	const auto Get = [&](EAugmentEquipSlot Slot, bool& bOcc, FItemInstance& Inst)
	{
		if (const FAugmentSlotState* S = Slots.Find(Slot))
		{
			bOcc = S->bOccupied;
			Inst = S->StoredInstance;
		}
	};

	Get(EAugmentEquipSlot::AugmentSlot1, Out.bS1, Out.Slot1);
	Get(EAugmentEquipSlot::AugmentSlot2, Out.bS2, Out.Slot2);
	Get(EAugmentEquipSlot::AugmentSlot3, Out.bS3, Out.Slot3);
}

FItemOp UAugmentEquipComponent::ImportSaveData(UInventorySubsystem* Inventory, const FAugmentEquipSaveData& In)
{
	if (!Inventory) return FItemOp::Fail("Reject.NoInventory");
	if (In.CharacterId != CharacterId) return FItemOp::Fail("Reject.CharacterIdMismatch");

	// 먼저 전부 비우기(인벤 복원 포함)
	for (auto& KV : Slots)
	{
		if (KV.Value.bOccupied)
		{
			const FItemOp Un = TryUnequipToInventory(Inventory, KV.Key);
			if (!Un.bOk) return Un;
		}
	}

	// 인벤에 존재해야 장착 가능. (세이브 로드 순서는 보통 인벤 로드 -> 장착 로드)
	auto EquipFromSave = [&](bool bOcc, const FItemInstance& Inst, EAugmentEquipSlot Slot) -> FItemOp
	{
		if (!bOcc) return FItemOp::Ok();
		// 인벤에서 해당 InstanceId를 찾는다
		FItemInstance Found;
		if (!Inventory->TryGetInstance(Inst.InstanceId, Found))
			return FItemOp::Fail("Reject.MissingInstanceInInventory");

		return TryEquipFromInventory(Inventory, Inst.InstanceId, Slot);
	};

	FItemOp R;
	R = EquipFromSave(In.bS1, In.Slot1, EAugmentEquipSlot::AugmentSlot1);
	if (!R.bOk) return R;
	R = EquipFromSave(In.bS2, In.Slot2, EAugmentEquipSlot::AugmentSlot2);
	if (!R.bOk) return R;
	R = EquipFromSave(In.bS3, In.Slot3, EAugmentEquipSlot::AugmentSlot3);
	if (!R.bOk) return R;

	return FItemOp::Ok();
}
