#include "Combat/Tactical/TacticalModeSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatParticipantInterface.h"

UBattleSessionSubsystem* UTacticalModeSubsystem::GetBattle() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

bool UTacticalModeSubsystem::IsPlayerTurnActor(AActor* Actor) const
{
	if (!Actor) return false;

	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Battle||!Battle->IsBattleActive()) return false;
	if (!Battle->CanActorActNow(Actor)) return false;

	ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor);
	if (!P) return false;

	return P->GetCombatTeam() == ECombatTeam::Player;
}

bool UTacticalModeSubsystem::TryEnterTacticalMode(AActor* Requester, FName ReasonTag)
{
	if (IsActive()) return false;
	if (!Requester) return false;
	if (!IsPlayerTurnActor(Requester)) return false;

	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Battle) return false;

	if (!Battle->PauseFlow("TacticalMode"))
		return false;

	Snapshot.BattleSessionId = Battle->GetSnapshot().SessionId;
	Snapshot.State = ETacticalModeState::Active;
	Snapshot.OperatorActor = Requester;
	Snapshot.EnterReason = ReasonTag;

	OnTacticalModeEntered.Broadcast(Snapshot);
	return true;
}

void UTacticalModeSubsystem::ExitTacticalMode(FName)
{
	if (!IsActive()) return;

	FTacticalModeSnapshot Final = Snapshot;

	if (UBattleSessionSubsystem*Battle =GetBattle())
	{
		Battle->ResumeFlow("TacticalMode");
	}

	Snapshot =FTacticalModeSnapshot();
	OnTacticalModeExited.Broadcast(Final);
}
