#include "Combat/Characters/CombatCharacterActor.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatLevelProviderComponent.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	CharacterComp = CreateDefaultSubobject<UCombatCharacterComponent>(TEXT("CombatCharacterComponent"));
	LevelProviderComp = CreateDefaultSubobject<UCombatLevelProviderComponent>(TEXT("CombatLevelProviderComponent"));

	HPComp = CreateDefaultSubobject<UHPComponent>(TEXT("HPComponent"));
	APComp = CreateDefaultSubobject<UAPComponent>(TEXT("APComponent"));
	SPComp = CreateDefaultSubobject<USPComponent>(TEXT("SPComponent"));
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();
	// CharacterComp가 BeginPlay에서 Def 기반 초기화를 수행
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
	return CompClass ?GetComponentByClass(CompClass) : nullptr;
}