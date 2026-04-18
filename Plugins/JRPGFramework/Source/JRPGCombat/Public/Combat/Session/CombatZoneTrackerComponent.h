#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatZoneTrackerComponent.generated.h"

class ACombatZoneActor;

UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UCombatZoneTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatZoneTrackerComponent();

	ACombatZoneActor* GetCurrentZone() const { return CurrentZone.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ACombatZoneActor> CurrentZone;

	UFUNCTION()
	void OnOwnerBeginOverlap(
		UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOwnerEndOverlap(
		UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyZoneToMovement(ACombatZoneActor* Zone);
};
