#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;

UCLASS()
class JRPG_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAIPerceptionComponent> PerceptionComp;
	UPROPERTY() TObjectPtr<UAISenseConfig_Sight> SightConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, struct FAIStimulus Stimulus);

	bool IsParty(AActor* Actor) const;
};
