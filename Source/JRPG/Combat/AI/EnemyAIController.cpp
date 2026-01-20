#include "EnemyAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "JRPG/Combat/Encounter/EncounterSubsystem.h"

AEnemyAIController::AEnemyAIController()
{
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerception");
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>("DamageConfig");

	SightConfig->SightRadius = 2500.f;
	SightConfig->LoseSightRadius = 3000.f;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->ConfigureSense(*DamageConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::HandleTargetPerceptionUpdated);
}

bool AEnemyAIController::IsParty(AActor* Actor) const
{
	return Actor && Actor->ActorHasTag("PartyMember");
}

void AEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed()) return;
	if (!IsParty(Actor)) return;

	APawn* EnemyPawn = GetPawn();
	if (!EnemyPawn) return;

	if (UEncounterSubsystem* Enc = GetWorld()->GetSubsystem<UEncounterSubsystem>())
		Enc->RequestEncounter(EnemyPawn, Actor, EEncounterTrigger::Sight);
}
