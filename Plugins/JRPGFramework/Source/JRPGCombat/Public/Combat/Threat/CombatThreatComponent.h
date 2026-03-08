#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "Combat/Threat/CombatThreatTypes.h"
#include "CombatThreatComponent.generated.h"

class UThreatConfigDataAsset;

/**
 * 어그로(Threat) SSOT (보통 "적" 캐릭터에 부착)
 * - ThreatTable: SourceActor -> Threat
 * - Tick: RealTime 기반 감쇠 + 죽은 대상 정리 + 타겟 재선정
 * - Hysteresis: 타겟 전환 히스테리시스(깜빡임 방지)
 * - ForceTarget(도발): 기간 동안 강제 타겟
 * - LockTarget: 일정 기간 타겟 전환 방지(그랩/연출 등)
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UCombatThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatThreatComponent();

	// Config
	UPROPERTY(EditDefaultsOnly, Category="JRPG|Threat")
	TObjectPtr<UThreatConfigDataAsset> Config = nullptr;

	// Events
	FOnThreatTargetChanged OnTargetChanged;
	FOnThreatTableChanged OnThreatTableChanged;

	// --- Inputs (기본전투/스킬에서 호출) ---
	FJRPGOpResult AddThreat(AActor* Source, float Amount, FName ReasonTag);
	FJRPGOpResult ReportThreatEvent(const FThreatEvent& Ev);

	// --- Control ---
	FJRPGOpResult ClearThreat(AActor* Source, FName ReasonTag);
	void ClearAllThreat(FName ReasonTag);

	// 도발/강제 타겟(기간)
	FJRPGOpResult ForceTarget(AActor* Target, float DurationSec, FName ReasonTag);
	void ClearForcedTarget(FName ReasonTag);

	// 타겟 전환 방지(기간)
	FJRPGOpResult LockTarget(float DurationSec, FName ReasonTag);
	void ClearLock(FName ReasonTag);

	// --- Query ---
	AActor* GetCurrentTarget() const { return EffectiveTarget.Get(); }
	AActor* GetForcedTarget() const { return ForcedTarget.Get(); }
	bool IsLocked() const { return bTargetLocked; }

	float GetThreat(AActor* Source) const;
	TArray<FThreatEntryRuntime> DebugGetThreatEntries(TArray<AActor*>& OutSources) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Threat table
	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AActor>, FThreatEntryRuntime> ThreatTable;

	// Targets
	UPROPERTY(Transient) TWeakObjectPtr<AActor> CurrentTarget;
	UPROPERTY(Transient) TWeakObjectPtr<AActor> EffectiveTarget; // ForcedTarget 있으면 ForcedTarget

	UPROPERTY(Transient) TWeakObjectPtr<AActor> ForcedTarget;
	FTimerHandle ForcedTargetTimer;

	bool bTargetLocked = false;
	FTimerHandle LockTimer;

	// Real-time tick
	double LastRealTime = 0.0;
	double LastSwitchRealTime = -1e9;

	// Internals
	const FThreatTuning& GetTuning() const;

	float ComputeThreatMultiplier(EThreatEventKind Kind) const;

	void TickDecayAndCleanup(float RealDelta);
	void RecomputeTargetIfNeeded(float NowRealTime);

	float ComputeScoreFor(AActor* Source) const;
	float DistanceMultiplier(AActor* Source) const;
	bool HasLineOfSightTo(AActor* Source) const;

	bool IsDeadActor(AActor* A) const;

	void SetCurrentTarget(AActor* NewTarget, FName ReasonTag);
	void RefreshEffectiveTarget(FName ReasonTag);

	void OnForcedTargetExpired();
	void OnLockExpired();
};