#include "Combat/AI/CombatCharacterActorAIController.h"

ACombatCharacterActorAIController::ACombatCharacterActorAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACombatCharacterActorAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	UE_LOG(LogTemp, Log, TEXT("CombatCharacterActorController : AI 빙의 -> %s"), *GetNameSafe(InPawn));
}

void ACombatCharacterActorAIController::OnUnPossess()
{
	UE_LOG(LogTemp, Log, TEXT("CombatCharacterActorController : AI 빙의 해제"));
	Super::OnUnPossess();
}
