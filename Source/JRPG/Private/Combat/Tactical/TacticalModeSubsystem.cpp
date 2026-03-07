#include "Combat/Tactical/TacticalModeSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatParticipantInterface.h"

UBattleSessionSubsystem* UTacticalModeSubsystem::GetBattle()const
{
	return GetWorld() ?GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

bool UTacticalModeSubsystem::IsPlayerTurnActor(AActor*Actor)const
{
	if (!Actor)
		return false;

	UBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle || !Battle->IsBattleActive())	return false;
	if (!Battle->CanActorActNow(Actor))			return false;

	ICombatParticipantInterface *P =Cast<ICombatParticipantInterface>(Actor);
	if (!P)
		return false;

	return P->GetCombatTeam() == ECombatTeam::Player;
}

bool UTacticalModeSubsystem::IsSessionParticipant(AActor *Actor) const
{
	if (!Actor)
		return false;

	UBattleSessionSubsystem *Battle = GetBattle();
	
	if (!Battle||!Battle->IsBattleActive()) 
		return false;

	TArray<AActor*> Alive;
	Battle->GetAliveParticipants(Alive);
	return Alive.Contains(Actor);
}

bool UTacticalModeSubsystem::TryEnterTacticalMode(AActor*Requester,FName ReasonTag)
{
	if (IsActive())return false;
	if (!Requester)return false;
	if (!IsPlayerTurnActor(Requester))return false;

	UBattleSessionSubsystem*Battle =GetBattle();
	if (!Battle)return false;

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
	if (!IsActive())return;

	FTacticalModeSnapshot Final =Snapshot;

	if (UBattleSessionSubsystem*Battle =GetBattle())
	{
		Battle->ResumeFlow("TacticalMode");
	}

	Snapshot =FTacticalModeSnapshot();
	OnTacticalModeExited.Broadcast(Final);
}

bool UTacticalModeSubsystem::SetReservation(AActor *Actor,FName SkillId,const TArray<AActor*> &Targets)
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

bool UTacticalModeSubsystem::ClearReservation(AActor *Actor)
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

bool UTacticalModeSubsystem::GetReservation(AActor*Actor,FTacticalReservation&OutReservation)const
{
	if (!Actor)
		return false;
	
	const FTacticalReservation *Found = Reservations.Find(Actor);
	
	if (!Found)
		return false;
	
	OutReservation = *Found;
	return true;
}

bool UTacticalModeSubsystem::HasReservation(AActor *Actor)const
{
	if (!Actor)
		return false;
	return Reservations.Contains(Actor);
}