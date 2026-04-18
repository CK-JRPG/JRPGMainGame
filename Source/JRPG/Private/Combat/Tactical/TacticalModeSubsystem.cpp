#include "Combat/Tactical/TacticalModeSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Infrastructure/CombatTimeSubsystem.h"

UBattleSessionSubsystem* UTacticalModeSubsystem::GetBattle()const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

UCombatTimeSubsystem* UTacticalModeSubsystem::GetTimeSubsystem() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatTimeSubsystem>() : nullptr;
}

bool UTacticalModeSubsystem::IsPlayerTurnActor(AActor* Actor)const
{
	if (!Actor)
		return false;

	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Battle || !Battle->IsBattleActive())	return false;
	if (!Battle->CanActorActNow(Actor))			return false;

	ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor);
	if (!P)
		return false;

	return P->GetCombatTeam() == ECombatTeam::Player;
}

bool UTacticalModeSubsystem::IsSessionParticipant(AActor* Actor) const
{
	if (!Actor)
		return false;

	UBattleSessionSubsystem* Battle = GetBattle();

	if (!Battle || !Battle->IsBattleActive())
		return false;

	TArray<AActor*> Alive;
	Battle->GetAliveParticipants(Alive);
	return Alive.Contains(Actor);
}

void UTacticalModeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UBattleSessionSubsystem>();
	Collection.InitializeDependency<UCombatTimeSubsystem>();

	if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		Battle->OnBattlePhaseChanged.AddUObject(this, &UTacticalModeSubsystem::OnBattlePhaseChanged);
	}
}

void UTacticalModeSubsystem::Deinitialize()
{
	if (IsActive())
	{
		ExitTacticalMode(TEXT("SubsystemDeinitialized"));
	}

	if (UWorld* World = GetWorld())
	{
		if (UBattleSessionSubsystem* Battle = World->GetSubsystem<UBattleSessionSubsystem>())
		{
			Battle->OnBattlePhaseChanged.RemoveAll(this);
		}
	}

	Super::Deinitialize();
}

bool UTacticalModeSubsystem::TryEnterTacticalMode(AActor* Requester, FName ReasonTag)
{
	UE_LOG(LogTemp, Warning, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : Try"));
	if (Snapshot.State != ETacticalModeState::Idle)
	{
		UE_LOG(LogTemp, Error, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : if (Snapshot.State != ETacticalModeState::Idle)"));
		return false;
	}
	if (!Requester) 
	{
		UE_LOG(LogTemp, Error, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : if (!Requester) "));
		return false;
	}

	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Battle)
	{
		UE_LOG(LogTemp, Error, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : if (!Battle) "));
		return false;
	}

	if (Battle->GetPhase() != EBattlePhase::Active)	
	{
		UE_LOG(LogTemp, Error, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : if (Battle->GetPhase() != EBattlePhase::Active)	"));
		return false;
	}

	ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Requester);
	if (!P || P->GetCombatTeam() != ECombatTeam::Player) 
	{
		UE_LOG(LogTemp, Error, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : if (!P || P->GetCombatTeam() != ECombatTeam::Player) "));
		return false;
	}

	UCombatTimeSubsystem* TimeSub = GetTimeSubsystem();
	if (!TimeSub) 
	{
		UE_LOG(LogTemp, Error, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : if (!TimeSub) "));
		return false;
	}

	Snapshot.State = ETacticalModeState::Entering;
	UE_LOG(LogTemp, Warning, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : Entering"));

	FCombatTimeRequest TimeReq;
	TimeReq.Mode = ECombatTimeMode::Slow;
	TimeReq.Priority = ECombatTimePriority::Medium;
	TimeReq.OwnerTag = TEXT("Tactical");
	TimeReq.TimeScale = 0.15f;        
	TimeReq.DurationRealSec = 5.0f;   
	TimeReq.BlendInSec = 0.10f;       
	TimeReq.BlendOutSec = 0.12f;

	FCombatTimeResult TimeRes = TimeSub->RequestTimeMode(TimeReq);

	if (!TimeRes.Handle.IsValid())
	{
		Snapshot.State = ETacticalModeState::Idle;
		UE_LOG(LogTemp, Warning, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : Time Request Rejected. Rollback to Idle."));
		return false;
	}

	TacticalTimeHandle = TimeRes.Handle;

	Snapshot.State = ETacticalModeState::Active;
	Snapshot.BattleSessionId = Battle->GetSnapshot().SessionId;
	Snapshot.OperatorActor = Requester;
	Snapshot.EnterReason = ReasonTag;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DurationTimerHandle, FTimerDelegate::CreateLambda([this]()
			{
				ExitTacticalMode(TEXT("DurationExpired"));
			}), TimeReq.DurationRealSec, false);
	}

	UE_LOG(LogTemp, Warning, TEXT("UTacticalModeSubsystem::TryEnterTacticalMode : Start TacticalMode"));
	OnTacticalModeEntered.Broadcast(Snapshot);
	return true;
}

void UTacticalModeSubsystem::ExitTacticalMode(FName ReasonTag)
{
	if (Snapshot.State != ETacticalModeState::Active && Snapshot.State != ETacticalModeState::Entering)
		return;

	Snapshot.State = ETacticalModeState::Exiting;

	if (UCombatTimeSubsystem* TimeSub = GetTimeSubsystem())
	{
		if (TacticalTimeHandle.IsValid())
		{
			TimeSub->ReleaseTimeMode(TacticalTimeHandle, ReasonTag);
			TacticalTimeHandle.Invalidate();
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	Snapshot.State = ETacticalModeState::Idle;

	FTacticalModeSnapshot Final = Snapshot;
	Final.EnterReason = ReasonTag; // 종료 사유 업데이트

	OnTacticalModeExited.Broadcast(Final);

	Snapshot = FTacticalModeSnapshot();
}

bool UTacticalModeSubsystem::SetReservation(AActor* Actor, FName SkillId, const TArray<AActor*>& Targets)
{
	if (!Actor || SkillId.IsNone())    return false;
	if (!IsSessionParticipant(Actor))  return false;

	FJRPGTacticalReservation R;
	R.ReservedActor = Actor;
	R.SkillId = SkillId;
	R.CreatedAtReal = FPlatformTime::Seconds();
	R.bQueued = true;

	for (AActor* T : Targets)
	{
		if (T)R.Targets.Add(T);
	}

	const bool bSameSkillToggle =
		Reservations.Contains(Actor) &&
		Reservations[Actor].SkillId == SkillId;

	if (bSameSkillToggle)
	{
		Reservations.Remove(Actor);
		OnTacticalReservationChanged.Broadcast(Actor, false, NAME_None);
		return true;
	}

	Reservations.Add(Actor, R);
	OnTacticalReservationChanged.Broadcast(Actor, true, SkillId);
	return true;
}

bool UTacticalModeSubsystem::ClearReservation(AActor* Actor)
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

bool UTacticalModeSubsystem::GetReservation(AActor* Actor, FJRPGTacticalReservation& OutReservation)const
{
	if (!Actor)
		return false;

	const FJRPGTacticalReservation* Found = Reservations.Find(Actor);

	if (!Found)
		return false;

	OutReservation = *Found;
	return true;
}

bool UTacticalModeSubsystem::HasReservation(AActor* Actor)const
{
	if (!Actor)
		return false;
	return Reservations.Contains(Actor);
}

void UTacticalModeSubsystem::OnBattlePhaseChanged(EBattlePhase NewPhase)
{
	if (NewPhase == EBattlePhase::Ending || NewPhase == EBattlePhase::Cleanup)
	{
		if (IsActive())
		{
			UE_LOG(LogTemp, Warning, TEXT("UTacticalModeSubsystem::OnBattlePhaseChanged : TacticalMode: Session Ending detected. Forced Exit."));
			ExitTacticalMode(TEXT("SessionEnded"));
		}
	}
}
