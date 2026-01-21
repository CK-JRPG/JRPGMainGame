#include "EnemyCharacter.h"

#include "JRPG/Combat/Stats/HealthComponent.h"
#include "JRPG/Combat/Threat/ThreatComponent.h"
#include "JRPG/Combat/Skill/SkillComponent.h"
#include "JRPG/Combat/AI/CombatAIComponent.h"
#include "JRPG/Combat/AI/EnemyAIController.h"

AEnemyCharacter::AEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    Health = CreateDefaultSubobject<UHealthComponent>("Health");
    Threat = CreateDefaultSubobject<UThreatComponent>("Threat");
    Skills = CreateDefaultSubobject<USkillComponent>("Skills");
    CombatAI = CreateDefaultSubobject<UCombatAIComponent>("CombatAI");

    Tags.AddUnique("Enemy");

    AIControllerClass = AEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
}
