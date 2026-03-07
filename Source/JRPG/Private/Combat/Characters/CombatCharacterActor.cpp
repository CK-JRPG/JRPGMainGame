#include "Combat/Characters/CombatCharacterActor.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/Stats/CombatStatsComponent.h"
#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Skills/SkillComponent.h"
#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

ACombatCharacterActor::ACombatCharacterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CharacterComp = CreateDefaultSubobject<UCombatCharacterComponent>(TEXT("CombatCharacterComponent"));

	HPComp = CreateDefaultSubobject<UHPComponent>(TEXT("HPComponent"));
	APComp = CreateDefaultSubobject<UAPComponent>(TEXT("APComponent"));
	SPComp = CreateDefaultSubobject<USPComponent>(TEXT("SPComponent"));

	StatsComp = CreateDefaultSubobject<UCombatStatsComponent>(TEXT("CombatStatsComponent"));
	ActionComp = CreateDefaultSubobject<UCombatActionComponent>(TEXT("CombatActionComponent"));
	SkillComp = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	StatusComp = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));
	GroggyComp = CreateDefaultSubobject<UGroggyComponent>(TEXT("GroggyComponent"));
	ThreatComp = CreateDefaultSubobject<UThreatComponent>(TEXT("ThreatComponent"));
}

void ACombatCharacterActor::BeginPlay()
{
	Super::BeginPlay();
}

FName ACombatCharacterActor::GetCombatantId() const
{
	return CharacterComp ? CharacterComp->GetCharacterId() : NAME_None;
}

ECombatTeam ACombatCharacterActor::GetCombatTeam() const
{
	return CharacterComp ? CharacterComp->GetTeam() : ECombatTeam::Neutral;
}

bool ACombatCharacterActor::IsPlayerControlledCombatant() const
{
	return IsPlayerControlled();
}

UActorComponent* ACombatCharacterActor::GetOptionalComponentByClass(TSubclassOf<UActorComponent> CompClass) const
{
	return CompClass ? GetComponentByClass(CompClass) : nullptr;
}