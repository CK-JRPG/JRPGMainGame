#pragma once

#include "CoreMinimal.h"
#include "EncounterTypes.generated.h"

class UCombatZoneSettingDataAsset;

UENUM(BlueprintType)
enum class EEncounterTrigger : uint8
{
	Sight   UMETA(DisplayName = "Sight"),
	Hit     UMETA(DisplayName = "Hit"),
};

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