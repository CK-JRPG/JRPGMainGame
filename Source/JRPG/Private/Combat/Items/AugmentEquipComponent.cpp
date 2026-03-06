// Source/JRPGCombat/Private/Combat/Items/AugmentEquipComponent.cpp
#include "Combat/Items/AugmentEquipComponent.h"
#include "Combat/Items/InventorySubsystem.h"
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
	if (const FAugmentSlotState* S = Slots.Find(Slot))
		return S->bOccupied;
	return false;
}

FAugmentSlotState UAugmentEquipComponent::GetSlotState(EAugmentEquipSlot Slot) const
{
	if (const FAugmentSlotState* S = Slots.Find(Slot))
		return *S;
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
		const UItemDataAsset* Def = FindDef(S.ItemId);
		if (!Def) continue;
		if (Def->UniqueEquipGroup == UniqueGroup)
			return true;
	}
	return false;
}

FItemOp UAugmentEquipComponent::ValidateEquip(const UItemDataAsset* Def, EAugmentEquipSlot Slot) const
{
	if (!Def) return FItemOp::Fail("Reject.ItemNotFound");
	if (!Def->IsAugment()) return FItemOp::Fail("Reject.ItemTypeNotAugment");
	if (!Def->IsSlotAllowed(Slot)) return FItemOp::Fail("Reject.SlotInvalid");

	// RequiredLevel: CharacterLevel < RequiredLevel이면 실패
	const int32 Required = Def->RequiredLevel;
	if (Required > 0 && LevelProvider)
	{
		const int32 CurLevel = LevelProvider->GetCharacterLevel(GetOwner());
		if (CurLevel < Required)
			return FItemOp::Fail("Reject.LevelTooLow");
	}

	// RoleRestriction(옵션)
	if (!Def->IsRoleAllowed(Role))
		return FItemOp::Fail("Reject.RoleNotAllowed");

	// Unique group conflict
	if (!Def->UniqueEquipGroup.IsNone() && HasUniqueGroupEquipped(Def->UniqueEquipGroup))
		return FItemOp::Fail("Reject.UniqueConflict");

	return FItemOp::Ok();
}

FItemOp UAugmentEquipComponent::TryEquipFromInventory(UInventorySubsystem* Inventory, FGuid InstanceId, EAugmentEquipSlot Slot)
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

	// 기존 아이템이 있으면 먼저 해제(인벤 공간 필요)
	if (IsSlotOccupied(Slot))
	{
		FItemOp Un = TryUnequipToInventory(Inventory, Slot);
		if (!Un.bOk)
		{
			OnAugmentEquipRejected.Broadcast(CharacterId, Inst.ItemId, Un.ReasonTag);
			return Un;
		}
	}

	// 장착: 인벤에서 인스턴스를 제거(Quantity는 Augment=1이 기본)
	// Augment MaxStack=1 SSOT
	const FItemOp Rem = Inventory->RemoveItemByInstance(InstanceId, 1, "Equip.ToSlot");
	if (!Rem.bOk)
	{
		OnAugmentEquipRejected.Broadcast(CharacterId, Inst.ItemId, Rem.ReasonTag);
		return Rem;
	}

	// 슬롯 갱신
	FAugmentSlotState& S = Slots.FindChecked(Slot);
	const FName OldItemId = S.ItemId;

	S.bOccupied = true;
	S.InstanceId = InstanceId;
	S.ItemId = Inst.ItemId;

	// Equipped tracking(상점 판매 제한)
	Inventory->NotifyEquipped(InstanceId);

	// Mods rebuild & notify
	RebuildModifierSet();
	OnModifierSetChanged.Broadcast(CharacterId);

	OnAugmentEquipped.Broadcast(CharacterId, Slot, OldItemId, S.ItemId, "Equip.Success");
	return FItemOp::Ok();
}

FItemOp UAugmentEquipComponent::TryUnequipToInventory(UInventorySubsystem* Inventory, EAugmentEquipSlot Slot)
{
	if (!Inventory)return FItemOp::Fail("Reject.NoInventory");

	FAugmentSlotState* S = Slots.Find(Slot);
	if (!S || !S->bOccupied)
		return FItemOp::Fail("Reject.SlotEmpty");

	// 인벤 공간 규칙: 인벤 부족이면 해제 불가
	int32 NeedSlots = 0;
	if (!Inventory->CanAcceptItem(S->ItemId, 1, &NeedSlots))
		return FItemOp::Fail("Reject.InventoryFull");

	// 인벤에 다시 추가(새 인스턴스로 만들면 GUID 유지가 깨짐)
	// -> SSOT: InstanceId 유지가 유리(강화/옵션 확장 대비). 그래서 "복구"를 지원.
	// InventorySubsystem은 현재 AddItem이 새 인스턴스를 생성하므로, 여기선 “복구용 내부 삽입” 대신
	// 안전하게 새 인스턴스 생성 정책을 택한다(강화/옵션을 쓰기 시작하면 Restore API 추가 권장).
	// 지금은 EnhanceLevel/Seed가 의미 없으니 괜찮음.
	const FItemOp Add = Inventory->AddItem(S->ItemId, 1, "Unequip.ToInventory", nullptr);
	if (!Add.bOk)
		return Add;

	// Equipped tracking 해제(기존 InstanceId는 “장착 슬롯에서만” 의미가 있었음)
	Inventory->NotifyUnequipped(S->InstanceId);

	const FName OldItemId = S->ItemId;
	S->bOccupied = false;
	S->InstanceId.Invalidate();
	S->ItemId = NAME_None;

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
			if (E.TargetSkillTag.IsNone())return;
			float& P = Map.FindOrAdd(E.TargetSkillTag);
			P += V;
			P = ApplyCapPct(E.CapGroup, P);
		};

	switch (E.EffectType)
	{
		case EAugmentEffectType::AttackFlat:CachedMods.Attack.Flat += V; break;
		case EAugmentEffectType::AttackPct:AddPct(CachedMods.Attack.Pct); break;
		case EAugmentEffectType::DefenseFlat:CachedMods.Defense.Flat += V; break;
		case EAugmentEffectType::DefensePct:AddPct(CachedMods.Defense.Pct); break;
		case EAugmentEffectType::HPFlat:CachedMods.HP.Flat += V; break;
		case EAugmentEffectType::HPPct:AddPct(CachedMods.HP.Pct); break;
		case EAugmentEffectType::BreakPowerFlat:CachedMods.BreakPower.Flat += V; break;
		case EAugmentEffectType::BreakPowerPct:AddPct(CachedMods.BreakPower.Pct); break;
		case EAugmentEffectType::HealingPowerFlat:CachedMods.HealingPower.Flat += V; break;
		case EAugmentEffectType::HealingPowerPct:AddPct(CachedMods.HealingPower.Pct); break;
		case EAugmentEffectType::ThreatModPct:AddPct(CachedMods.ThreatModPct); break;

		case EAugmentEffectType::AoERadiusPct:AddSkillPct(CachedMods.AoERadius.PctBySkillTag); break;
		case EAugmentEffectType::BuffDurationPct:AddSkillPct(CachedMods.BuffDuration.PctBySkillTag); break;
		case EAugmentEffectType::DebuffPotencyPct:AddSkillPct(CachedMods.DebuffPotency.PctBySkillTag); break;
		case EAugmentEffectType::BreakBuildUpPct:AddSkillPct(CachedMods.BreakBuildUp.PctBySkillTag); break;

		default: break;
	}
}

void UAugmentEquipComponent::RebuildModifierSet()
{
	CachedMods = FAugmentModifierSet();

	for (const auto& KV : Slots)
	{
		const FAugmentSlotState& S = KV.Value;
		if (!S.bOccupied)continue;

		const UItemDataAsset* Def = FindDef(S.ItemId);
		if (!Def)continue;

		// RoleEfficiency 기본 적용 (RoleRestriction이 있어도 효율은 적용 가능)
		const float RoleEff = Def->RoleEfficiency.Get(Role);

		for (const FAugmentEffect& E : Def->EffectList)
		{
			ApplyEffectWithCaps(Def, E, RoleEff);
		}
	}
}