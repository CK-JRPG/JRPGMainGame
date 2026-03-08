#include"Combat/Characters/CombatCharacterComponent.h"
#include"Combat/Characters/CombatCharacterDataAsset.h"
#include"Combat/Characters/CombatCharacterRegistrySubsystem.h"

#include"Combat/Skills/JRPGSkillComponent.h"
#include "Combat/Skills/JRPGSkillDataAsset.h"

#if __has_include("Combat/Items/InventorySubsystem.h")
	#include "Combat/Items/InventorySubsystem.h"
	#define JRPG_HAS_INVENTORY 1
#else
	#define JRPG_HAS_INVENTORY 0
#endif

UCombatCharacterComponent::UCombatCharacterComponent()
{
	PrimaryComponentTick.bCanEverTick=false;
}

void UCombatCharacterComponent::BeginPlay()
{
	Super::BeginPlay();
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
	GiveStartingItems();
	GiveStartingSkills();
}

void UCombatCharacterComponent::GiveStartingItems()
{
#if JRPG_HAS_INVENTORY
	if (!CharacterDef)
		return;
	
	if (!GetWorld()||!GetWorld()->GetGameInstance())
		return;

	if (UInventorySubsystem *Inv = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		for (constauto &KV :CharacterDef -> StartingItems)
		{
			if (!KV.Key.IsNone() && KV.Value>0)
				Inv->AddItem(KV.Key, KV.Value,"Character.StartingItems", nullptr);
		}
	}
#endif
}

void UCombatCharacterComponent::GiveStartingSkills()
{
	if (!CharacterDef) return;
	if (UJRPGSkillComponent *SC =GetOwner() ? GetOwner()->FindComponentByClass<UJRPGSkillComponent>() : nullptr)
	{
		// 여기선 SkillId만 있으므로 실제 SkillDataAsset 매핑은 프로젝트에서 AssetManager로 연결.
		// SkillId 목록을 유지만 해두고, 런타임에 외부에서 LearnSkill로 주입해도 됨.
	}
}

void UCombatCharacterComponent::RegisterToRegistry()
{
	if (CharacterId.IsNone()) return;
	if (!GetWorld()||!GetWorld()->GetGameInstance()) return;
	if (UCombatCharacterRegistrySubsystem *Reg =GetWorld()->GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>())
		Reg->RegisterCharacter(CharacterId,GetOwner());
}

void UCombatCharacterComponent::UnregisterFromRegistry()
{
	if (CharacterId.IsNone()) 
		return;
	
	if (!GetWorld()||!GetWorld()->GetGameInstance()) 
		return;
	
	if (UCombatCharacterRegistrySubsystem *Reg = GetWorld()->GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>())
		Reg->UnregisterCharacter(CharacterId, GetOwner());
}