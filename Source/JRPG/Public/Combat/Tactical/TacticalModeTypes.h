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
struct FTacticalModeSnapshot
{
	GENERATED_BODY()

	UPROPERTY() FGuid BattleSessionId;
	UPROPERTY() ETacticalModeState State = ETacticalModeState::Inactive;

	UPROPERTY() TWeakObjectPtr<AActor> OperatorActor;
	UPROPERTY() FName EnterReason =NAME_None;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTacticalModeEntered, const FTacticalModeSnapshot& /*Snapshot*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTacticalModeExited, const FTacticalModeSnapshot& /*Snapshot*/);
