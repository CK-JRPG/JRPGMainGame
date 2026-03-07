#include "Combat/Characters/CombatCharacterActor.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/Stats/CombatStatsComponent.h"
#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Skills/SkillComponent.h"
#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"
#include "Combat/AI/CombatAIActionSelectorComponent.h"
#include "Combat/Items/CombatItemComponent.h"
#include "Combat/Presentation/CombatPresentationComponent.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	CharacterComp =CreateDefaultSubobject<UCombatCharacterComponent>(TEXT("CombatCharacterComponent"));

	HPComp = CreateDefaultSubobject<UHPComponent>(TEXT("HPComponent"));
	APComp = CreateDefaultSubobject<UAPComponent>(TEXT("APComponent"));
	SPComp = CreateDefaultSubobject<USPComponent>(TEXT("SPComponent"));

	StatsComp = CreateDefaultSubobject<UCombatStatsComponent>(TEXT("CombatStatsComponent"));
	ActionComp = CreateDefaultSubobject<UCombatActionComponent>(TEXT("CombatActionComponent"));
	SkillComp = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	StatusComp = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));
	GroggyComp = CreateDefaultSubobject<UGroggyComponent>(TEXT("GroggyComponent"));
	ThreatComp = CreateDefaultSubobject<UThreatComponent>(TEXT("ThreatComponent"));
	AIActionSelectorComp = CreateDefaultSubobject<UCombatAIActionSelectorComponent>(TEXT("CombatAIActionSelectorComponent"));
	ItemComp = CreateDefaultSubobject<UCombatItemComponent>(TEXT("CombatItemComponent"));
	PresentationComp = CreateDefaultSubobject<UCombatPresentationComponent>(TEXT("CombatPresentationComponent"));
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

