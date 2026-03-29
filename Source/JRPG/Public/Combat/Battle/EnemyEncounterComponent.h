
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyEncounterComponent.generated.h"

class ACombatZoneActor;
class USphereComponent;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class JRPG_API UEnemyEncounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEnemyEncounterComponent();
	
protected:
	virtual void BeginPlay() override;
	
private:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
						  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						  bool bFromSweep, const FHitResult& SweepResult);
	
	void SearchCombatCharactersInRadius(const AActor* PlayerActor);
	void ReadyForBattleSession(const FBattleSessionConfig& Config);
	void CreateCombatZone();
		
	
public:
	
	UPROPERTY(EditAnywhere, Category = "Encounter")
	float DetectionRadius = 150.0f;
	
	UPROPERTY(EditAnywhere, Category = "Encounter")
	float EnemySearchRadius = 1000.0f;
	
	UPROPERTY(EditAnywhere, Category = "Encounter")
	TSubclassOf<ACombatZoneActor> CombatZoneClass;
	
private:
	UPROPERTY()
	TObjectPtr<USphereComponent> TriggerSphere;
	
	UPROPERTY()
	TObjectPtr<ACombatZoneActor> SpawnedZone;
	
	bool bHasTriggered = false;
	

		
};
