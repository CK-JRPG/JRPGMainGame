#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

#include "Combat/Items/InventorySubsystem.h"

UCombatCharacterComponent::UCombatCharacterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatCharacterComponent::BeginPlay()
{
	Super::BeginPlay();

	HP = GetOwner() ? GetOwner()->FindComponentByClass<UHPComponent>() : nullptr;
	AP = GetOwner() ? GetOwner()->FindComponentByClass<UAPComponent>() : nullptr;
	SP = GetOwner() ? GetOwner()->FindComponentByClass<USPComponent>() : nullptr;

	InitializeFromDef();
	RegisterToRegistry();
}

void UCombatCharacterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromRegistry();
	Super::EndPlay(EndPlayReason);
}

void UCombatCharacterComponent::InitializeFromDef()
{
	if (!CharacterDef || !CharacterDef->IsValidDef())
		return;

	CharacterId = CharacterDef->CharacterId;
	Team = CharacterDef->DefaultTeam;
	Role = CharacterDef->DefaultRole;

	ApplyBaseParamsToResources();
	GiveStartingItems();
}

void UCombatCharacterComponent::ApplyBaseParamsToResources()
{
	if (!CharacterDef) return;

	const FCharacterBaseParams& P =CharacterDef->BaseParams;

	if (HP) HP->InitializeHP(P.MaxHP, true);
	if (AP) AP->InitializeAP(P.MaxAP, true);
	if (SP) SP->InitializeSP(P.MaxSP, 0);
}

void UCombatCharacterComponent::GiveStartingItems()
{
	if (!CharacterDef) return;
	if (!GetWorld() || !GetWorld()->GetGameInstance()) return;

	if (UInventorySubsystem* Inv = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		for (const auto& KV :CharacterDef->StartingItems)
		{
			const FName ItemId = KV.Key;
			const int32 Qty = KV.Value;
			if (!ItemId.IsNone() && Qty > 0)
			{
				Inv->AddItem(ItemId, Qty, "Character.StartingItems", nullptr);
			}
		}
	}
}

void UCombatCharacterComponent::RegisterToRegistry()
{
	if (CharacterId.IsNone()) return;
	if (!GetWorld()||!GetWorld()->GetGameInstance()) return;

	if (UCombatCharacterRegistrySubsystem* Reg = GetWorld()->GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>())
	{
		Reg->RegisterCharacter(CharacterId, GetOwner());
	}
}

void UCombatCharacterComponent::UnregisterFromRegistry()
{
	if (CharacterId.IsNone()) return;
	if (!GetWorld() || !GetWorld()->GetGameInstance()) return;

	if (UCombatCharacterRegistrySubsystem* Reg = GetWorld()->GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>())
	{
		Reg->UnregisterCharacter(CharacterId, GetOwner());
	}
}