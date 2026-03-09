#pragma once

#include "CoreMinimal.h"
#include "TacticalModeTypes.generated.h"

UENUM()
enum class ETacticalModeState : uint8
{
	Inactive,
	Active
};

USTRUCT()
struct FJRPGTacticalReservation
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> ReservedActor;
	UPROPERTY() FName SkillId = NAME_None;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> Targets;

	UPROPERTY() double CreatedAtReal = 0.0;
	UPROPERTY() bool bQueued = false;
};

USTRUCT()
struct FTacticalModeSnapshot
{
	GENERATED_BODY()

	UPROPERTY() FGuid BattleSessionId;
	UPROPERTY() ETacticalModeState State = ETacticalModeState::Inactive;

	UPROPERTY() TWeakObjectPtr<AActor> OperatorActor;
	UPROPERTY() FName EnterReason = NAME_None;

};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTacticalModeEntered, const FTacticalModeSnapshot& /*Snapshot*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTacticalModeExited, const FTacticalModeSnapshot& /*Snapshot*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTacticalReservationChanged, AActor* /*Actor*/, bool /*bHasReservation*/,FName /*SkillId*/);

