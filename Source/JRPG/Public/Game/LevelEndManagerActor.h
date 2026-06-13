#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelEndManagerActor.generated.h"

class ACombatCharacterActor;
class APlayerController;
class ULevelEndUIWidget;

UCLASS(Blueprintable)
class JRPG_API ALevelEndManagerActor : public AActor
{
	GENERATED_BODY()

public:
	ALevelEndManagerActor();

	UFUNCTION(BlueprintCallable, Category = "JRPG|LevelEnd")
	void InitializeTrackedEnemies();

	UFUNCTION(BlueprintCallable, Category = "JRPG|LevelEnd")
	void TriggerLevelEnd();

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JRPG|LevelEnd")
	TSubclassOf<ULevelEndUIWidget> LevelEndWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JRPG|LevelEnd|Tracking", meta = (ClampMin = "0.0"))
	float InitialTrackingDelay = 0.2f;

private:
	UPROPERTY()
	TArray<TObjectPtr<ACombatCharacterActor>> TrackedEnemies;

	UPROPERTY()
	TObjectPtr<ULevelEndUIWidget> LevelEndWidget;

	FTimerHandle InitialTrackingTimerHandle;
	bool bLevelEndTriggered = false;
	int32 RemainingEnemyCount = 0;

	void HandleEnemyDeath(AActor* Killer, FName ReasonTag);
	bool IsEnemyAlive(const ACombatCharacterActor* Enemy) const;
	void ApplyLevelEndInputMode(APlayerController* PC);
	void EnsureLevelEndCursorVisible() const;
	void UnbindTrackedEnemies();
};
