#pragma once

#include "CoreMinimal.h"
#include "BattleSessionTypes.h"
#include "Combat/Battle/EncounterTypes.h"
#include "GameFramework/Actor.h"
#include "EncounterTriggerActor.generated.h"

class ACombatCharacterActor;
class AJRPGPlayerPawn;
class UBoxComponent;
class ACombatZoneActor; 
class UCombatZoneSettingDataAsset;

UCLASS()
class JRPG_API AEncounterTriggerActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEncounterTriggerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	UBoxComponent* TriggerVolume;

	// Zone 세팅 DA (CombatZoneClass, 크기 등)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Zone")
	TSubclassOf<ACombatZoneActor> CombatZoneClass;

	TObjectPtr<UCombatZoneSettingDataAsset> ZoneSetting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	float MaxEncounterCompanionDistance = 1200.0f;

	// 런타임에 생성된 존을 기억해두기 위한 포인터 (전투 종료 시 파괴하기 위함)
	UPROPERTY(Transient)
	ACombatZoneActor* SpawnedZone;

	bool bHasTriggered = false;

private:

	FTransform CompanionFallbackTransform(const AActor* LeaderActor, const class AJRPGCompanionPawn* Companion, int32 CompanionOrder) const;
	void SearchCombatCharactersInRadius(const AActor* OverlapActor);
	bool ReadyforBattleSession(const FBattleSessionConfig& Config, const FEncounterContext& InEncounterCtx);
	void OnPlayerApproach();

};
