#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "Combat/Time/CombatTimeTypes.h"
#include "TacticalModeSubsystem.generated.h"

class UBattleSessionSubsystem;
class UCombatTimeSubsystem;

UCLASS()
class JRPG_API UTacticalModeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FOnTacticalModeEntered OnTacticalModeEntered;
	FOnTacticalModeExited OnTacticalModeExited;
	FOnTacticalReservationChanged OnTacticalReservationChanged;

	bool IsActive()const { return Snapshot.State == ETacticalModeState::Active; }
	const FTacticalModeSnapshot& GetSnapshot()const { return Snapshot; }

	bool TryEnterTacticalMode(AActor* Requester, FName ReasonTag);
	void ExitTacticalMode(FName ReasonTag);

	bool SetReservation(AActor* Actor, FName SkillId, const TArray<AActor*>& Targets);
	bool ClearReservation(AActor* Actor);
	bool GetReservation(AActor* Actor, FJRPGTacticalReservation& OutReservation) const;
	bool HasReservation(AActor* Actor)const;

	UFUNCTION()
	void OnBattlePhaseChanged(EBattlePhase NewPhase);

private:
	UPROPERTY() FTacticalModeSnapshot Snapshot;
	UPROPERTY() TMap<TWeakObjectPtr<AActor>, FJRPGTacticalReservation> Reservations;

	UPROPERTY() FCombatTimeHandle TacticalTimeHandle;
	FTimerHandle DurationTimerHandle;

	UBattleSessionSubsystem* GetBattle() const;
	UCombatTimeSubsystem* GetTimeSubsystem() const;
	void ClearAllReservations();

	bool IsPlayerTurnActor(AActor* Actor) const;
	bool IsSessionParticipant(AActor* Actor) const;
};
