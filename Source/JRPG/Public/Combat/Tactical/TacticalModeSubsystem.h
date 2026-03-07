#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "TacticalModeSubsystem.generated.h"

class UBattleSessionSubsystem;

UCLASS()
class JRPG_API UTacticalModeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnTacticalModeEntered OnTacticalModeEntered;
	FOnTacticalModeExited OnTacticalModeExited;
	FOnTacticalReservationChanged OnTacticalReservationChanged;

	bool IsActive()const { return Snapshot.State == ETacticalModeState::Active; }
	const FTacticalModeSnapshot& GetSnapshot()const { return Snapshot; }

	bool TryEnterTacticalMode(AActor *Requester,FName ReasonTag);
	void ExitTacticalMode(FName ReasonTag);

	bool SetReservation(AActor *Actor,FName SkillId,const TArray<AActor*> &Targets);
	bool ClearReservation(AActor *Actor);
	bool GetReservation(AActor *Actor,FTacticalReservation &OutReservation) const;
	bool HasReservation(AActor *Actor)const;

private:
	UPROPERTY() FTacticalModeSnapshot Snapshot;
	UPROPERTY() TMap<TWeakObjectPtr<AActor>, FTacticalReservation> Reservations;

	UBattleSessionSubsystem *GetBattle() const;
	bool IsPlayerTurnActor(AActor *Actor) const;
	bool IsSessionParticipant(AActor *Actor) const;
};

