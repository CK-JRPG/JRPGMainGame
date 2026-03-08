#include "Combat/Characters/CombatCharacterActor.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/Stats/CombatStatsComponent.h"
#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Skills/JRPGSkillComponent.h"
#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Groggy/CombatGroggyComponent.h"
#include "Combat/Threat/CombatThreatComponent.h"
#include "Combat/AI/CombatAIActionSelectorComponent.h"
#include "Combat/Items/CombatItemComponent.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Motion/JRPGCombatMotionComponent.h"

#include "Combat/Stats/CombatHPComponent.h"
#include "Combat/Stats/CombatAPComponent.h"
#include "Combat/SP/SPComponent.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	CharacterComp =CreateDefaultSubobject<UCombatCharacterComponent>(TEXT("CombatCharacterComponent"));

	HPComp = CreateDefaultSubobject<UCombatHPComponent>(TEXT("HPComponent"));
	APComp = CreateDefaultSubobject<UCombatAPComponent>(TEXT("APComponent"));
	SPComp = CreateDefaultSubobject<USPComponent>(TEXT("SPComponent"));

	StatsComp = CreateDefaultSubobject<UCombatStatsComponent>(TEXT("CombatStatsComponent"));
	ActionComp = CreateDefaultSubobject<UCombatActionComponent>(TEXT("CombatActionComponent"));
	SkillComp = CreateDefaultSubobject<UJRPGSkillComponent>(TEXT("SkillComponent"));
	StatusComp = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));
	GroggyComp = CreateDefaultSubobject<UGroggyComponent>(TEXT("GroggyComponent"));
	ThreatComp = CreateDefaultSubobject<UCombatThreatComponent>(TEXT("ThreatComponent"));
	AIActionSelectorComp = CreateDefaultSubobject<UCombatAIActionSelectorComponent>(TEXT("CombatAIActionSelectorComponent"));
	ItemComp = CreateDefaultSubobject<UCombatItemComponent>(TEXT("CombatItemComponent"));
	PresentationComp = CreateDefaultSubobject<UCombatPresentationComponent>(TEXT("CombatPresentationComponent"));
	MotionComp = CreateDefaultSubobject<UJRPGCombatMotionComponent>(TEXT("CombatMotionComponent"));
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();
}

FName ACombatCharacter::GetCombatantId() const
{
	return CharacterComp ? CharacterComp->GetCharacterId() : NAME_None;
}

ECombatTeam ACombatCharacter::GetCombatTeam() const
{
	return CharacterComp ? CharacterComp->GetTeam() : ECombatTeam::Neutral;
}

bool ACombatCharacter::IsPlayerControlledCombatant() const
{
	return IsPlayerControlled();
}

UActorComponent* ACombatCharacter::GetOptionalComponentByClass(TSubclassOf<UActorComponent> CompClass) const
{
	return CompClass ? GetComponentByClass(CompClass) : nullptr;
}

