#include "Combat/Tactical/TacticalModeSubsystem.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Kismet/GameplayStatics.h"

// Framework 연동 헤더
#include "Combat/Infrastructure/CombatTimeSubsystem.h"
#include "Combat/Session/CombatZoneTrackerComponent.h"

void UTacticalModeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Snapshot = FTacticalModeSnapshot();
	TacticalTimeHandle.Invalidate();
	ActiveStartReal = 0.0;
	bWasInCombatZoneLastTick = false;
	Reservations.Reset();
}

void UTacticalModeSubsystem::Deinitialize()
{
	if (TacticalTimeHandle.IsValid())
	{
		if (UCombatTimeSubsystem* TimeSub = GetWorld()->GetSubsystem<UCombatTimeSubsystem>())
		{
			TimeSub->ReleaseTimeMode(TacticalTimeHandle, "Tactical.Deinit");
		}
	}
	TacticalTimeHandle.Invalidate();
	Super::Deinitialize();
}

// 플레이어가 전투 구역(Zone) 안에 있는지 확인
bool UTacticalModeSubsystem::IsPlayerInCombatZone() const
{
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		if (UCombatZoneTrackerComponent* Tracker = PlayerPawn->FindComponentByClass<UCombatZoneTrackerComponent>())
		{
			return Tracker->GetCurrentZone() != nullptr;
		}
	}
	return false;
}

// 특정 액터가 전투 구역(Zone) 안에 있는지 확인
bool UTacticalModeSubsystem::IsActorInCombatZone(AActor* Actor) const
{
	if (!Actor) return false;
	if (UCombatZoneTrackerComponent* Tracker = Actor->FindComponentByClass<UCombatZoneTrackerComponent>())
	{
		return Tracker->GetCurrentZone() != nullptr;
	}
	return false;
}

bool UTacticalModeSubsystem::TryEnterTacticalMode(AActor* Requester, FName ReasonTag)
{
	if (IsActive()) return false;
	if (!Requester) return false;

	if (!IsPlayerInCombatZone()) 
	{
		UE_LOG(LogTemp, Error, TEXT("실패 원인: 플레이어가 전투 구역(Zone) 안에 없습니다!"));
		return false;
	}

	ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Requester);
	if (!P || P->GetCombatTeam() != ECombatTeam::Player) 
	{
		UE_LOG(LogTemp, Error, TEXT("실패 원인: 캐릭터가 Player 팀 인터페이스를 구현하지 않았습니다!"));
		return false;
	}

	// Framework의 CombatTimeSubsystem에 슬로모(15%) 요청
	if (UCombatTimeSubsystem* TimeSub = GetWorld()->GetSubsystem<UCombatTimeSubsystem>())
	{
		FCombatTimeRequest Req;
		Req.OwnerTag = "Tactical";
		Req.TimeScale = 0.15f; 
		Req.DurationRealSec = 5.0f; 
		// Req.Mode = ECombatTimeMode::Slow; // (Framework Enum에 맞춰 주석 해제)
		
		FCombatTimeResult TR = TimeSub->RequestTimeMode(Req);
		if (TR.Op.bOk)
		{
			TacticalTimeHandle = TR.Handle;
		}
		else
		{
			return false; // 시간 요청 실패 시 진입 불가
		}
	}

	Snapshot.State = ETacticalModeState::Active;
	Snapshot.OperatorActor = Requester;
	Snapshot.EnterReason = ReasonTag;
	ActiveStartReal = GetWorld()->GetRealTimeSeconds();

	OnTacticalModeEntered.Broadcast(Snapshot);
	return true;
}

void UTacticalModeSubsystem::ExitTacticalMode(FName ReasonTag)
{
	if (!IsActive()) return;

	// Framework의 시간 모드 해제
	if (TacticalTimeHandle.IsValid())
	{
		if (UCombatTimeSubsystem* TimeSub = GetWorld()->GetSubsystem<UCombatTimeSubsystem>())
		{
			TimeSub->ReleaseTimeMode(TacticalTimeHandle, ReasonTag);
		}
		TacticalTimeHandle.Invalidate();
	}

	FTacticalModeSnapshot Final = Snapshot;
	Snapshot = FTacticalModeSnapshot(); // Inactive로 초기화

	OnTacticalModeExited.Broadcast(Final);
}

bool UTacticalModeSubsystem::SetReservation(AActor* Actor, FName SkillId, const TArray<AActor*>& Targets)
{
	if (!Actor || SkillId.IsNone()) return false;
	
	// Session 대신 Zone 참가 여부 검사
	if (!IsActorInCombatZone(Actor)) return false;

	FJRPGTacticalReservation R;
	R.ReservedActor = Actor;
	R.SkillId = SkillId;
	R.CreatedAtReal = FPlatformTime::Seconds();
	R.bQueued = true;

	for (AActor* T : Targets)
	{
		if (T) R.Targets.Add(T);
	}

	const bool bSameSkillToggle = Reservations.Contains(Actor) && Reservations[Actor].SkillId == SkillId;

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
	if (!Actor) return false;
	
	const bool bRemoved = Reservations.Remove(Actor) > 0;
	if (bRemoved)
	{
		OnTacticalReservationChanged.Broadcast(Actor, false, NAME_None);
	}
	return bRemoved;
}

bool UTacticalModeSubsystem::GetReservation(AActor* Actor, FJRPGTacticalReservation& OutReservation) const
{
	if (!Actor) return false;
	
	if (const FJRPGTacticalReservation* Found = Reservations.Find(Actor))
	{
		OutReservation = *Found;
		return true;
	}
	return false;
}

bool UTacticalModeSubsystem::HasReservation(AActor* Actor) const
{
	return Actor && Reservations.Contains(Actor);
}

void UTacticalModeSubsystem::HandleCombatZoneStateChanged()
{
	const bool bActiveZone = IsPlayerInCombatZone();

	if (bWasInCombatZoneLastTick && !bActiveZone)
	{
		if (IsActive())
		{
			ExitTacticalMode(FName("Tactical.ForcedExit.LeftZone"));
		}
	}
	bWasInCombatZoneLastTick = bActiveZone;
}

void UTacticalModeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetWorld()) return;

	HandleCombatZoneStateChanged();

	if (IsActive())
	{
		const double Now = GetWorld()->GetRealTimeSeconds();
		if (Now - ActiveStartReal >= 5.0) // 기획서의 최대 5초 제한 구현
		{
			ExitTacticalMode(FName("Tactical.DurationExpired"));
		}
	}
}