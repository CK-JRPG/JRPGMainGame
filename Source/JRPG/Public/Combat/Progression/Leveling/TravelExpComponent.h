// Source/JRPGCombat/Public/Combat/Progression/Leveling/TravelExpComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TravelExpComponent.generated.h"

class ULevelingSubsystem;
class UExpSettingsDataAsset;

UCLASS(ClassGroup=(Progression), meta=(BlueprintSpawnableComponent))
class JRPG_API UTravelExpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTravelExpComponent();

	// 설정(없으면 LevelingSubsystem.ExpSettings를 사용)
	UPROPERTY(EditAnywhere)
	TObjectPtr<UExpSettingsDataAsset> SettingsOverride = nullptr;

	UPROPERTY(EditAnywhere)
	float SampleIntervalSec = 0.25f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle Timer;

	TArray<int32> CellHistory;
	FVector LastLoc = FVector::ZeroVector;

	float AccumTimeSec = 0.f;
	float AccumDistanceCm = 0.f;

	void Sample();

	ULevelingSubsystem* GetLeveling() const;
	const UExpSettingsDataAsset* GetSettings() const;

	int32 HashCell(const FVector& Loc, float Grid) const;
	bool DetectBacktrackABA() const;
	bool IsSameAreaFarm(const FVector& Loc, float RadiusCm) const;

	void PushCell(int32 CellHash, int32 MaxSize);
};
