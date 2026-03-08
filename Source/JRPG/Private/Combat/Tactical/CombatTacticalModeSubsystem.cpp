#include "Combat/Tactical/CombatTacticalModeSubsystem.h"

#include "Combat/Battle/CombatBattleSessionSubsystem.h"
#include "Combat/Characters/CombatParticipantInterface.h"

UCombatBattleSessionSubsystem* UCombatTacticalModeSubsystem::GetBattle()const
{
	return GetWorld() ?GetWorld()->GetSubsystem<UCombatBattleSessionSubsystem>() : nullptr;
}

bool UCombatTacticalModeSubsystem::IsPlayerTurnActor(AActor*Actor)const
{
	if (!Actor)
		return false;

	UCombatBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle || !Battle->IsBattleActive())	return false;
	if (!Battle->CanActorActNow(Actor))			return false;

	ICombatParticipantInterface *P =Cast<ICombatParticipantInterface>(Actor);
	if (!P)
		return false;

	return P->GetCombatTeam() == ECombatTeam::Player;
}

bool UCombatTacticalModeSubsystem::IsSessionParticipant(AActor *Actor) const
{
	if (!Actor)
		return false;

	UCombatBattleSessionSubsystem *Battle = GetBattle();
	
	if (!Battle||!Battle->IsBattleActive()) 
		return false;

	TArray<AActor*> Alive;
	Battle->GetAliveParticipants(Alive);
	return Alive.Contains(Actor);
}

bool UCombatTacticalModeSubsystem::TryEnterTacticalMode(AActor*Requester,FName ReasonTag)
{
	if (IsActive()) return false;
	if (!Requester) return false;

	UCombatBattleSessionSubsystem*Battle =GetBattle();
	if (!Battle) 
		return false;
	
	if (Battle->GetPhase()!= EBattlePhase::Active)
		return false;

	ICombatParticipantInterface*P =Cast<ICombatParticipantInterface>(Requester);
	if (!P||P->GetCombatTeam()!= ECombatTeam::Player) 
		return false;

	Snapshot.BattleSessionId = Battle->GetSnapshot().SessionId;
	Snapshot.State = ETacticalModeState::Active;
	Snapshot.OperatorActor = Requester;
	Snapshot.EnterReason = ReasonTag;

	OnTacticalModeEntered.Broadcast(Snapshot);
	return true;
}

void UCombatTacticalModeSubsystem::ExitTacticalMode(FName)
{
	if (!IsActive())return;

	FTacticalModeSnapshot Final = Snapshot;
	Snapshot = FTacticalModeSnapshot();

	OnTacticalModeExited.Broadcast(Final);
}

bool UCombatTacticalModeSubsystem::SetReservation(AActor *Actor,FName SkillId,const TArray<AActor*> &Targets)
{
	if (!Actor || SkillId.IsNone())    return false;
	if (!IsSessionParticipant(Actor))  return false;

	FTacticalReservation R;
	R.ReservedActor = Actor;
	R.SkillId = SkillId;
	R.CreatedAtReal = FPlatformTime::Seconds();
	R.bQueued = true;

	for (AActor *T : Targets)
	{
		if (T)R.Targets.Add(T);
	}

	const bool bSameSkillToggle =
	Reservations.Contains(Actor)&&
	Reservations[Actor].SkillId==SkillId;

	if (bSameSkillToggle)
	{
		Reservations.Remove(Actor);
		OnTacticalReservationChanged.Broadcast(Actor,false,NAME_None);
		return true;
	}

	Reservations.Add(Actor,R);
	OnTacticalReservationChanged.Broadcast(Actor, true, SkillId);
	return true;
}

bool UCombatTacticalModeSubsystem::ClearReservation(AActor *Actor)
{
	if (!Actor)
		return false;
	
	const bool bRemoved = Reservations.Remove(Actor) > 0;
	if (bRemoved)
	{
		OnTacticalReservationChanged.Broadcast(Actor, false, NAME_None);
	}
	return bRemoved;
}

bool UCombatTacticalModeSubsystem::GetReservation(AActor*Actor,FTacticalReservation&OutReservation)const
{
	if (!Actor)
		return false;
	
	const FTacticalReservation *Found = Reservations.Find(Actor);
	
	if (!Found)
		return false;
	
	OutReservation = *Found;
	return true;
}

bool UCombatTacticalModeSubsystem::HasReservation(AActor *Actor)const
{
	if (!Actor)
		return false;
	return Reservations.Contains(Actor);
}

