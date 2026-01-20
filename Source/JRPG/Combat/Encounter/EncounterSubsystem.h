#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EncounterSubsystem.generated.h"

UENUM()
enum class EEncounterTrigger : uint8
{
	Sight,
	Hit
};

UCLASS()
class JRPG_API UEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Encounter") float EncounterRadiusMeters = 100.f;

	// TriggerEnemy: 전투를 유발한 적(메인 타겟 우선)
	// TriggerActor: 플레이어(또는 공격자)
	UFUNCTION() void RequestEncounter(AActor* TriggerEnemy, AActor* TriggerActor, EEncounterTrigger Trigger);

private:
	void CollectActors(const FVector& CenterCm, float RadiusCm, TArray<AActor*>& OutParty, TArray<AActor*>& OutEnemies) const;
};

