#include "Combat/Infrastructure/CombatTacticalModeSubsystem.h"

#include "Combat/Tactical/TacticalSettingsDataAsset.h"
#include "Combat/Time/CombatTimeSubsystem.h"
#include "Combat/Skills/JRPGSkillComponent.h"
#include "Combat/Stats/CombatHPComponent.h"
#include "Combat/Infrastructure/CombatBattleSessionSubsystem.h"

UCombatTimeSubsystem* UCombatTacticalModeSubsystem::GetTimeSubsystem() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatTimeSubsystem>() : nullptr;
}

UCombatBattleSessionSubsystem* UCombatTacticalModeSubsystem::GetBattleSession() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatBattleSessionSubsystem>() : nullptr;
}

void UCombatTacticalModeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Settings = SettingsAsset ? SettingsAsset->Settings : FTacticalSettings();

	Settings.TacticalMaxDurationRealSec = FMath::Clamp(Settings.TacticalMaxDurationRealSec, 0.5f, 5.0f);
	Settings.TacticalSlowScale = FMath::Clamp(Settings.TacticalSlowScale, 0.05f, 0.30f);

	State = ETacticalState::Idle;
	TacticalTimeHandle.Invalidate();
	ActiveStartReal = 0.0;

	Reservations.Reset();

	WasBattleActiveLastTick = false;

	LastTimerBroadcastReal = -1e9;
	LastBroadcastRemaining = -1.f;
}

void UCombatTacticalModeSubsystem::Deinitialize()
{
	if (TacticalTimeHandle.IsValid())
	{
		if (UCombatTimeSubsystem* Time = GetTimeSubsystem())
		{
			Time->ReleaseTimeMode(TacticalTimeHandle, "Tactical.Deinit");
		}
	}
	TacticalTimeHandle.Invalidate();
	State = ETacticalState::Idle;

	Super::Deinitialize();
}

void UCombatTacticalModeSubsystem::TransitionTo(ETacticalState NewState)
{
	if (State == NewState) return;

	const ETacticalState Prev = State;
	State = NewState;
	OnTacticalStateChanged.Broadcast(Prev, NewState);

	// UI timer event also updates active flag
	OnTacticalTimerUpdated.Broadcast(GetElapsedRealSec(), GetRemainingRealSec(), GetNormalized01(), IsActive());
}

bool UCombatTacticalModeSubsystem::GuardSessionActive(FJRPGReason& OutReason) const
{
	UCombatBattleSessionSubsystem* Battle = GetBattleSession();
	if (!Battle || !Battle->IsCombatRunning())
	{
		OutReason = FJRPGReason::Make("SessionNotActive");
		return false;
	}
	return true;
}

bool UCombatTacticalModeSubsystem::GuardIdle(FJRPGReason& OutReason) const
{
	if (State != ETacticalState::Idle)
	{
		OutReason = FJRPGReason::Make("Tactical.NotIdle");
		return false;
	}
	return true;
}

FJRPGOpResult UCombatTacticalModeSubsystem::TryEnterTactical()
{
	FJRPGReason Reason;

	if (!GuardSessionActive(Reason))
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, Reason);

	if (!GuardIdle(Reason))
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, Reason);

	TransitionTo(ETacticalState::Entering);

	UCombatTimeSubsystem* Time = GetTimeSubsystem();
	if (!Time)
	{
		TransitionTo(ETacticalState::Idle);
		return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("TimeSubsystemMissing"));
	}

	FCombatTimeRequest Req;
	Req.Mode = ECombatTimeMode::Slow;
	Req.Priority = ECombatTimePriority::Medium;
	Req.OwnerTag = "Tactical";
	Req.TimeScale = Settings.TacticalSlowScale;
	Req.DurationRealSec = Settings.TacticalMaxDurationRealSec;
	Req.BlendInSec = Settings.BlendInSec;
	Req.BlendOutSec = Settings.BlendOutSec;

	FCombatTimeResult TR = Time->RequestTimeMode(Req);
	if (!TR.Op.bOk || !TR.Handle.IsValid())
	{
		TransitionTo(ETacticalState::Idle);
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("TimeRequestRejected"));
	}

	TacticalTimeHandle = TR.Handle;

	TransitionTo(ETacticalState::Active);

	ActiveStartReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	LastTimerBroadcastReal = ActiveStartReal;
	LastBroadcastRemaining = GetRemainingRealSec();

	// immediate timer update for UI
	OnTacticalTimerUpdated.Broadcast(GetElapsedRealSec(), GetRemainingRealSec(), GetNormalized01(), true);

	return FJRPGOpResult::Ok();
}

FJRPGOpResult UCombatTacticalModeSubsystem::TryExitTactical(FName ReasonTag)
{
	if (State == ETacticalState::Idle) return FJRPGOpResult::Ok();

	TransitionTo(ETacticalState::Exiting);

	if (TacticalTimeHandle.IsValid())
	{
		if (UCombatTimeSubsystem* Time = GetTimeSubsystem())
		{
			Time->ReleaseTimeMode(TacticalTimeHandle, ReasonTag.IsNone() ? "Tactical.Exit" : ReasonTag);
		}
		TacticalTimeHandle.Invalidate();
	}

	if (!Settings.bKeepReservationsOnExit)
	{
		ClearAllReservations("Tactical.ExitClear");
	}

	TransitionTo(ETacticalState::Idle);
	return FJRPGOpResult::Ok();
}

bool UCombatTacticalModeSubsystem::ValidateReservationActor(AActor* Actor, FJRPGReason& OutReason) const
{
	if (!Actor)
	{
		OutReason = FJRPGReason::Make("Tactical.ActorNull");
		return false;
	}

	if (UCombatBattleSessionSubsystem* Battle = GetBattleSession())
	{
		if (!Battle->IsParticipant(Actor))
		{
			OutReason = FJRPGReason::Make("Tactical.ActorNotParticipant");
			return false;
		}
	}

	return true;
}

bool UCombatTacticalModeSubsystem::ValidateReservationSkill(AActor* Actor, FName SkillId, FJRPGReason& OutReason) const
{
	if (SkillId.IsNone())
	{
		OutReason = FJRPGReason::Make("Tactical.SkillIdNone");
		return false;
	}

	UJRPGSkillComponent* Skill = Actor ? Actor->FindComponentByClass<UJRPGSkillComponent>() : nullptr;
	if (!Skill)
	{
		OutReason = FJRPGReason::Make("Tactical.NoSkillComponent");
		return false;
	}

	if (!Skill->HasSkill(SkillId))
	{
		OutReason = FJRPGReason::Make("Tactical.SkillNotOwned");
		return false;
	}

	return true;
}

FJRPGOpResult UCombatTacticalModeSubsystem::SetReservation(AActor* Actor, FName SkillId, const FTacticalTargetSnapshot& Target)
{
	FJRPGReason Reason;

	if (!GuardSessionActive(Reason))
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, Reason);

	if (!ValidateReservationActor(Actor, Reason))
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, Reason);

	if (!ValidateReservationSkill(Actor, SkillId, Reason))
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, Reason);

	FTacticalReservation* Existing = Reservations.Find(Actor);

	if (!Existing)
	{
		FTacticalReservation R;
		R.Actor = Actor;
		R.SkillId = SkillId;
		R.Target = Target;
		R.CreatedAtReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
		R.SetFlags(ETacticalReservationFlags::Queued);

		Reservations.Add(Actor, R);
		OnTacticalReservationChanged.Broadcast(Actor, true, SkillId);
		OnTacticalReservationFlagsChanged.Broadcast(Actor, SkillId, R.GetFlags());
		return FJRPGOpResult::Ok();
	}

	if (Existing->SkillId == SkillId && Settings.bToggleSameSkillClears)
	{
		Reservations.Remove(Actor);
		OnTacticalReservationChanged.Broadcast(Actor, false, NAME_None);
		return FJRPGOpResult::Ok();
	}

	Existing->SkillId = SkillId;
	Existing->Target = Target;
	Existing->CreatedAtReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	Existing->SetFlags(ETacticalReservationFlags::Queued);

	OnTacticalReservationChanged.Broadcast(Actor, true, SkillId);
	OnTacticalReservationFlagsChanged.Broadcast(Actor, SkillId, Existing->GetFlags());
	return FJRPGOpResult::Ok();
}

FJRPGOpResult UCombatTacticalModeSubsystem::ClearReservation(AActor* Actor, FName /*ReasonTag*/)
{
	if (!Actor)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Tactical.Clear.ActorNull"));

	const int32 Removed = Reservations.Remove(Actor);
	if (Removed <= 0)
		return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Tactical.Clear.NotFound"));

	OnTacticalReservationChanged.Broadcast(Actor, false, NAME_None);
	return FJRPGOpResult::Ok();
}

void UCombatTacticalModeSubsystem::ClearAllReservations(FName /*ReasonTag*/)
{
	for (const auto& It : Reservations)
	{
		AActor* A = It.Key.Get();
		if (A)
		{
			OnTacticalReservationChanged.Broadcast(A, false, NAME_None);
		}
	}
	Reservations.Reset();
}

bool UCombatTacticalModeSubsystem::GetReservation(AActor* Actor, FTacticalReservation& OutRes) const
{
	if (!Actor) return false;

	if (const FTacticalReservation* R = Reservations.Find(Actor))
	{
		OutRes = *R;
		return true;
	}
	return false;
}

void UCombatTacticalModeSubsystem::SetReservationFlags(AActor* Actor, ETacticalReservationFlags Flags)
{
	if (!Actor) return;

	if (FTacticalReservation* R = Reservations.Find(Actor))
	{
		if (R->GetFlags() != Flags)
		{
			R->SetFlags(Flags);
			OnTacticalReservationFlagsChanged.Broadcast(Actor, R->SkillId, Flags);
		}
	}
}

bool UCombatTacticalModeSubsystem::IsReservationTargetInvalid(AActor* Target)
{
	if (!Target) return true;
	if (UCombatHPComponent* HP = Target->FindComponentByClass<UCombatHPComponent>())
	{
		return HP->IsDead();
	}
	return false;
}

void UCombatTacticalModeSubsystem::TickDuration(double NowReal)
{
	if (State != ETacticalState::Active) return;

	const double Elapsed = NowReal - ActiveStartReal;
	if (Elapsed >= (double)Settings.TacticalMaxDurationRealSec)
	{
		TryExitTactical("Tactical.DurationExpired");
	}
}

void UCombatTacticalModeSubsystem::HandleBattlePhaseChanged()
{
	UCombatBattleSessionSubsystem* Battle = GetBattleSession();
	const bool bActive = Battle && Battle->IsCombatRunning();

	if (WasBattleActiveLastTick && !bActive)
	{
		if (State == ETacticalState::Active || State == ETacticalState::Entering)
		{
			TryExitTactical("Tactical.ForcedExit.SessionEnd");

			if (Settings.bClearReservationsOnForcedExit)
			{
				ClearAllReservations("Tactical.ForcedExitClear");
			}
		}
	}

	WasBattleActiveLastTick = bActive;
}

// ---------- UI Query API ----------
float UCombatTacticalModeSubsystem::GetElapsedRealSec() const
{
	if (!GetWorld()) return 0.f;
	if (State != ETacticalState::Active) return 0.f;

	const double Now = GetWorld()->GetRealTimeSeconds();
	return (float)FMath::Max(0.0, Now - ActiveStartReal);
}

float UCombatTacticalModeSubsystem::GetRemainingRealSec() const
{
	if (!GetWorld()) return 0.f;
	if (State != ETacticalState::Active) return 0.f;

	const float Elapsed = GetElapsedRealSec();
	return FMath::Max(0.f, Settings.TacticalMaxDurationRealSec - Elapsed);
}

float UCombatTacticalModeSubsystem::GetNormalized01() const
{
	if (State != ETacticalState::Active) return 0.f;
	const float MaxD = FMath::Max(0.001f, Settings.TacticalMaxDurationRealSec);
	return FMath::Clamp(GetElapsedRealSec() / MaxD, 0.f, 1.f);
}

bool UCombatTacticalModeSubsystem::GetReservationView(AActor* Actor, FTacticalReservationView& Out) const
{
	if (!Actor) return false;

	const FTacticalReservation* R = Reservations.Find(Actor);
	if (!R) return false;

	FTacticalReservationView V;
	V.Actor = R->Actor;
	V.SkillId = R->SkillId;
	V.Flags = R->GetFlags();
	V.CreatedAtReal = R->CreatedAtReal;

	V.TargetKind = R->Target.Kind;
	V.TargetActor = R->Target.TargetActor;
	V.TargetLocation = R->Target.TargetLocation;

	Out = V;
	return true;
}

void UCombatTacticalModeSubsystem::GetReservationsForUI(TArray<FTacticalReservationView>& Out) const
{
	Out.Reset();
	Out.Reserve(Reservations.Num());

	for (const auto& It : Reservations)
	{
		const FTacticalReservation& R = It.Value;

		FTacticalReservationView V;
		V.Actor = R.Actor;
		V.SkillId = R.SkillId;
		V.Flags = R.GetFlags();
		V.CreatedAtReal = R.CreatedAtReal;

		V.TargetKind = R.Target.Kind;
		V.TargetActor = R.Target.TargetActor;
		V.TargetLocation = R.Target.TargetLocation;

		Out.Add(V);
	}

	Out.Sort([](const FTacticalReservationView& A, const FTacticalReservationView& B)
	{
		return A.CreatedAtReal < B.CreatedAtReal;
	});
}

void UCombatTacticalModeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* W = GetWorld();
	if (!W) return;

	const double Now = W->GetRealTimeSeconds();

	HandleBattlePhaseChanged();
	TickDuration(Now);

	// timer update broadcast (throttle to ~20Hz, and only while active)
	if (State == ETacticalState::Active)
	{
		const float Remaining = GetRemainingRealSec();
		const bool bShouldTimeBroadcast =
			((Now - LastTimerBroadcastReal) >= 0.05) || (LastBroadcastRemaining < 0.f) || !FMath::IsNearlyEqual(Remaining, LastBroadcastRemaining, 0.01f);

		if (bShouldTimeBroadcast)
		{
			LastTimerBroadcastReal = Now;
			LastBroadcastRemaining = Remaining;

			OnTacticalTimerUpdated.Broadcast(GetElapsedRealSec(), Remaining, GetNormalized01(), true);
		}
	}
}
