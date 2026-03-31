#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "Combat/Time/CombatTimeTypes.h" // Framework의 시간 타입 사용
#include "TacticalModeSubsystem.generated.h"

UCLASS()
class JRPG_API UTacticalModeSubsystem : public UTickableWorldSubsystem // 5초 체크를 위해 Tickable로 변경
{
	GENERATED_BODY()

public:
	// 기존 델리게이트 유지
	FOnTacticalModeEntered OnTacticalModeEntered;
	FOnTacticalModeExited OnTacticalModeExited;
	FOnTacticalReservationChanged OnTacticalReservationChanged;

	bool IsActive() const { return Snapshot.State == ETacticalModeState::Active; }
	const FTacticalModeSnapshot& GetSnapshot() const { return Snapshot; }

	// API 유지
	bool TryEnterTacticalMode(AActor* Requester, FName ReasonTag);
	void ExitTacticalMode(FName ReasonTag);

	bool SetReservation(AActor* Actor, FName SkillId, const TArray<AActor*>& Targets);
	bool ClearReservation(AActor* Actor);
	bool GetReservation(AActor* Actor, FJRPGTacticalReservation& OutReservation) const;
	bool HasReservation(AActor* Actor) const;

	// Tickable World Subsystem 오버라이드
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTacticalModeSubsystem, STATGROUP_Tickables); }

private:
	UPROPERTY() FTacticalModeSnapshot Snapshot;
	UPROPERTY() TMap<TWeakObjectPtr<AActor>, FJRPGTacticalReservation> Reservations;

	// 프레임워크 연동용 시간 핸들 및 타이머 변수
	FCombatTimeHandle TacticalTimeHandle;
	double ActiveStartReal = 0.0;
	bool bWasInCombatZoneLastTick = false;

	// Zone 판정 헬퍼 함수
	bool IsPlayerInCombatZone() const;
	bool IsActorInCombatZone(AActor* Actor) const;
	void HandleCombatZoneStateChanged();
};