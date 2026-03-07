#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JRPGCoreApiTypes.h"
#include "Combat/Tactical/TacticalSettingsDataAsset.h"
#include "Combat/Tactical/TacticalTypes.h"
#include "Combat/Time/CombatTimeTypes.h"
#include "TacticalModeSubsystem.generated.h"

class UTacticalSettingsDataAsset;
class UCombatTimeSubsystem;
class UBattleSessionSubsystem;
struct

UCLASS()
class JRPGCOMBAT_API UTacticalModeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY() TObjectPtr<UTacticalSettingsDataAsset> SettingsAsset = nullptr;

	// events
	FOnTacticalStateChanged OnTacticalStateChanged;
	FOnTacticalReservationChanged OnTacticalReservationChanged;
	FOnTacticalReservationFlagsChanged OnTacticalReservationFlagsChanged;

	// ✅ UI timer event (optional)
	FOnTacticalTimerUpdated OnTacticalTimerUpdated;

	ETacticalState GetState() const { return State; }
	bool IsActive() const { return State == ETacticalState::Active; }

	// Entry/Exit
	FJRPGOpResult TryEnterTactical();
	FJRPGOpResult TryExitTactical(FName ReasonTag);

	// Reservation API
	FJRPGOpResult SetReservation(AActor* Actor, FName SkillId, const FTacticalTargetSnapshot& Target);
	FJRPGOpResult ClearReservation(AActor* Actor, FName ReasonTag);
	void ClearAllReservations(FName ReasonTag);

	bool GetReservation(AActor* Actor, FTacticalReservation& OutRes) const;
	void SetReservationFlags(AActor* Actor, ETacticalReservationFlags Flags);

	static bool IsReservationTargetInvalid(AActor* Target);

	// ---------- ✅ UI Query API ----------
	float GetMaxDurationRealSec() const { return Settings.TacticalMaxDurationRealSec; }

	/** Active일 때만 의미 있음. Idle이면 0 */
	float GetElapsedRealSec() const;

	/** Active일 때만 의미 있음. Idle이면 0 */
	float GetRemainingRealSec() const;

	/** 0~1 (Active가 아니면 0) */
	float GetNormalized01() const;

	/** 예약 전체를 UI용 View로 반환(정렬: CreatedAtReal 오름차순) */
	void GetReservationsForUI(TArray<FTacticalReservationView>& Out) const;

	/** 예약 개수 */
	int32 GetReservationCount() const { return Reservations.Num(); }

	/** 해당 Actor 예약의 UI View 반환 */
	bool GetReservationView(AActor* Actor, FTacticalReservationView& Out) const;

	// Tickable
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTacticalModeSubsystem, STATGROUP_Tickables); }

private:
	ETacticalState State = ETacticalState::Idle;

	// runtime settings
	FTacticalSettings Settings;

	// time handle
	FCombatTimeHandle TacticalTimeHandle;

	// duration (real time)
	double ActiveStartReal = 0.0;

	// reservations
	TMap<TWeakObjectPtr<AActor>, FTacticalReservation> Reservations;

	// cached subsystems
	UCombatTimeSubsystem* GetTimeSubsystem() const;
	UBattleSessionSubsystem* GetBattleSession() const;

	// state helpers
	void TransitionTo(ETacticalState NewState);

	// forced exit on session end
	void HandleBattlePhaseChanged();
	bool WasBattleActiveLastTick = false;

	// guards
	bool GuardSessionActive(FJRPGReason& OutReason) const;
	bool GuardIdle(FJRPGReason& OutReason) const;

	// validation
	bool ValidateReservationActor(AActor* Actor, FJRPGReason& OutReason) const;
	bool ValidateReservationSkill(AActor* Actor, FName SkillId, FJRPGReason& OutReason) const;

	// duration check
	void TickDuration(double NowReal);

	// ✅ UI timer event throttling
	double LastTimerBroadcastReal = -1e9;
	float LastBroadcastRemaining = -1.f;
};