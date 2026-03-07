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

	bool IsActive() const { return Snapshot.State == ETacticalModeState::Active; }
	const FTacticalModeSnapshot& GetSnapshot() const { return Snapshot; }

	bool TryEnterTacticalMode(AActor* Requester, FName ReasonTag);
	void ExitTacticalMode(FName ReasonTag);

private:
	UPROPERTY() FTacticalModeSnapshot Snapshot;

	UBattleSessionSubsystem* GetBattle() const;
	bool IsPlayerTurnActor(AActor* Actor) const;
};
