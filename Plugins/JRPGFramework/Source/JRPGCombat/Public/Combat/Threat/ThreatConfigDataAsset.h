#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Threat/CombatThreatTypes.h"
#include "ThreatConfigDataAsset.generated.h"

UENUM()
enum class EThreatDecayMode : uint8
{
	None,
	Linear,       // Threat = max(0, Threat - DecayPerSec * dt)
	Exponential   // Threat *= exp(-K * dt)
};

USTRUCT()
struct FThreatTuning
{
	GENERATED_BODY()

	// ---- decay ----
	UPROPERTY(EditDefaultsOnly) EThreatDecayMode DecayMode = EThreatDecayMode::Exponential;
	UPROPERTY(EditDefaultsOnly) float LinearDecayPerSec = 2.f;     // Linear
	UPROPERTY(EditDefaultsOnly) float ExpDecayK = 0.12f;           // Exponential

	// ---- target switching hysteresis ----
	UPROPERTY(EditDefaultsOnly) float SwitchMinIntervalSec = 0.35f; // 최소 전환 간격
	UPROPERTY(EditDefaultsOnly) float SwitchRatio = 1.15f;          // BestScore가 CurrentScore * Ratio 이상이면 전환
	UPROPERTY(EditDefaultsOnly) float SwitchAdditive = 5.f;         // BestScore가 CurrentScore + Additive 이상이면 전환

	// ---- distance / LOS scoring ----
	UPROPERTY(EditDefaultsOnly) bool bUseDistanceWeight = true;
	UPROPERTY(EditDefaultsOnly) float DistanceFalloffScale = 1200.f;  // 클수록 거리 영향 약함
	UPROPERTY(EditDefaultsOnly) float MinDistanceMultiplier = 0.35f;  // 너무 멀면 최소 배율

	UPROPERTY(EditDefaultsOnly) bool bUseLineOfSight = false;
	UPROPERTY(EditDefaultsOnly) float NoLOSMultiplier = 0.6f;
	UPROPERTY(EditDefaultsOnly) TEnumAsByte<ECollisionChannel> LOSChannel = ECC_Visibility;

	// ---- dead target cleanup ----
	UPROPERTY(EditDefaultsOnly) bool bRemoveDeadTargets = true;

	// ---- event multipliers ----
	UPROPERTY(EditDefaultsOnly) float Mult_DamageTaken = 1.0f;
	UPROPERTY(EditDefaultsOnly) float Mult_HealDone = 0.75f;
	UPROPERTY(EditDefaultsOnly) float Mult_BuffDone = 0.5f;
	UPROPERTY(EditDefaultsOnly) float Mult_Direct = 1.0f;

	// ---- debug ----
	UPROPERTY(EditDefaultsOnly) bool bVerboseLog = false;
};

UCLASS()
class JRPGCOMBAT_API UThreatConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly) FThreatTuning Tuning;
};