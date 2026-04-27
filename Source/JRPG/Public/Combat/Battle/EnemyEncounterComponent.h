#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Battle/EncounterTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "EnemyEncounterComponent.generated.h"

class UCombatZoneSettingDataAsset;
class ACombatZoneActor;
class USphereComponent;
class UPartySubsystem;
class UPartyActorSpawnSubsystem;
struct FBattleSessionConfig;
struct FBattleSessionSnapshot;



UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JRPG_API UEnemyEncounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyEncounterComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


private:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

    FEncounterContext BuildEncounterContext(const AActor* InTriggerActor);
	FTransform CompanionFallbackTransform(const AActor* LeaderActor, const class AJRPGCompanionPawn* Companion, int32 CompanionOrder) const;
	void SearchCombatEnemyCharactersInRadius(const AActor* PlayerActor);
	void ReadyForBattleSession(const FBattleSessionConfig& Config, const FEncounterContext& InEncounterCtx);

protected:
	void HandleBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);
	void ResetEncounterForRematch();


public:

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float DetectionRadius = 70.0f;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float EnemySearchRadius = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float MaxEncounterCompanionDistance = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Encounter|Zone")
	TObjectPtr<UCombatZoneSettingDataAsset> ZoneSetting;

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> ActiveEncounterEnemies;


	bool bHasTriggered = false;



};
