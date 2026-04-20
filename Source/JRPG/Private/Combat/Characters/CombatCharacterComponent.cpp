#include"Combat/Characters/CombatCharacterComponent.h"

#include"Combat/Characters/CombatCharacterDataAsset.h"
#include"Combat/Characters/CombatCharacterRegistrySubsystem.h"

#include"Combat/Skills/SkillComponent.h"
#include "Combat/Skills/SkillDataAsset.h"

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
		for (const auto &KV :CharacterDef -> StartingItems)
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

	USkillComponent* SC = GetOwner() ? GetOwner()->FindComponentByClass<USkillComponent>() : nullptr;
	if (!SC) return;

	// StartingSkills 배열(DA 직접 레퍼런스)로 스킬 등록
	for (USkillDataAsset* Skill : CharacterDef->StartingSkills)
	{
		if (Skill)
		{
			SC->LearnSkill(Skill);
		}
	}

	// StartingSkillIds(FName) fallback: 이미 KnownSkills에 있는 DA에서 매칭
	for (const FName& SkillId : CharacterDef->StartingSkillIds)
	{
		if (SkillId.IsNone()) continue;
		if (SC->HasSkill(SkillId)) continue;

		// KnownSkills(에디터에서 직접 설정된 DA)에서 ID 매칭 시도
		if (USkillDataAsset* Found = SC->GetSkillDef(SkillId))
		{
			SC->LearnSkill(Found);
		}
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