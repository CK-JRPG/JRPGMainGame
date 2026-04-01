#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyEncounterComponent.generated.h"


UENUM(BlueprintType)
enum class EEncounterTrigger : uint8
{
    Sight   UMETA(DisplayName = "Sight"),
    Hit     UMETA(DisplayName = "Hit"),
};

class UCombatZoneSettingDataAsset;

USTRUCT(BlueprintType)
struct FEncounterContext
{
    GENERATED_BODY()

    // Trigger 
    UPROPERTY(VisibleAnywhere, Category = "EncounterContext | Trigger")
    EEncounterTrigger Trigger = EEncounterTrigger::Sight;

    // Actors
    TObjectPtr<AActor> PrimaryEnemy;
    TArray<TWeakObjectPtr<AActor>> AssistEnemies;
    TObjectPtr<AActor> TriggerActor;

    // Zone
    UPROPERTY(VisibleAnywhere, Category = "EncounterContext | Zone")
    FVector ZoneCenter = FVector::ZeroVector;

    UPROPERTY(EditAnywhere)
    TObjectPtr<UCombatZoneSettingDataAsset> ZoneSetting;

    // Timestamp
    UPROPERTY(EditAnywhere, Category = "EncounterContext | Timestamp")
    double TimestampReal = 0.0;  

    // Token
    UPROPERTY(VisibleAnywhere, Category = "EncounterContext | Token")
    FGuid EncounterToken;
};

class ACombatZoneActor;
class USphereComponent;
class UPartySubsystem;
class UPartyActorSpawnSubsystem;
struct FBattleSessionConfig;



UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
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

    FEncounterContext BuildEncounterContext(const AActor* InTriggerActor);
	void SearchCombatEnemyCharactersInRadius(const AActor* PlayerActor);
	void ReadyForBattleSession(const FBattleSessionConfig& Config, FEncounterContext& InEncounterCtx);
	void CreateCombatZone(FEncounterContext& InEncounterCtx);


public:

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float DetectionRadius = 70.0f;

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