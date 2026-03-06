#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BondWalkComponent.generated.h"

class UBondSubsystem;
class UBondSettingsDataAsset;

UCLASS(ClassGroup=(Progression), meta=(BlueprintSpawnableComponent))
class JRPG_API UBondWalkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBondWalkComponent();

	UPROPERTY(EditAnywhere) TObjectPtr<UBondSettingsDataAsset> SettingsOverride = nullptr;
	UPROPERTY(EditAnywhere) float SampleIntervalSec = 0.25f;
	UPROPERTY(EditAnywhere) float MinMoveSpeedCmPerSec = 10.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle Timer;

	FVector LastLoc = FVector::ZeroVector;
	float AccumTimeSec = 0.f;
	float AccumDistanceCm = 0.f;

	void Sample();

	UBondSubsystem* GetBond() const;
	const UBondSettingsDataAsset* GetSettings() const;

	const TArray<FName>* GetPartyIds() const;
};