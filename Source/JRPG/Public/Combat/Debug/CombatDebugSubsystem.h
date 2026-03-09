#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/Debug/CombatDebugTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "Combat/Chain/ChainAttackTypes.h"
#include "Combat/SP/SynergyPointTypes.h"

#include "CombatDebugSubsystem.generated.h"

UCLASS()
class JRPG_API UCombatDebugSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) bool bOverlayEnabled = true;
	UPROPERTY(EditAnywhere) bool bEchoToOutputLog = true;
	UPROPERTY(EditAnywhere) int32 MaxEntries = 300;
	UPROPERTY(EditAnywhere) int32 OverlayMaxLines = 18;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	void AddLog(
		ECombatDebugCategory Category,
		FName Tag,
		const FString& Message,
		AActor* Instigator = nullptr,
		AActor* Target = nullptr,
		FLinearColor Color = FLinearColor::White);

	void ClearLogs();

	bool IsOverlayEnabled() const { return bOverlayEnabled; }
	void SetOverlayEnabled(bool bEnabled) { bOverlayEnabled = bEnabled; }
	void ToggleOverlay() { bOverlayEnabled =! bOverlayEnabled; }

	const TArray<FCombatDebugEntry>& GetEntries() const { return Entries; }

	void GetRecentEntries(int32 MaxCount, TArray<FCombatDebugEntry>& OutEntries) const;

private:
	UPROPERTY() TArray<FCombatDebugEntry> Entries;

	void BindGlobalEvents();

	// battle
	void HandleBattleStarted(const FBattleSessionSnapshot& Snapshot);
	void HandleBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);
	void HandleBattlePhaseChanged(EBattlePhase NewPhase);
	void HandleActorActionLockChanged(AActor* Actor, bool bLocked, FName ReasonTag);
	void HandleExclusiveModeChanged(bool bActive, FName ModeTag);

	// action
	void HandleBasicAttackResolved(const FCombatActionResult& Result);
	void HandleCombatantDefeated(AActor* Victim,AActor* Killer);

	// tactical
	void HandleTacticalEntered(const FTacticalModeSnapshot& Snapshot);
	void HandleTacticalExited(const FTacticalModeSnapshot& Snapshot);
	void HandleTacticalReservationChanged(AActor* Actor, bool bHasReservation, FName SkillId);

	// chain
	void HandleChainStarted(const FChainAttackSnapshot& Snapshot);
	void HandleChainStepResolved(const FChainAttackSnapshot& Snapshot, AActor* ActingActor);
	void HandleChainEnded(const FChainAttackSnapshot& Snapshot);

	// sp
	void HandleSPChanged(int32 CurrentSP, int32 Delta, EJRPGSPEventType Type, FName ReasonTag);
	void HandleSPReadyChanged(bool bReady);
	void HandleSPGainApplied(const FJRPGSPGainEvent& Event);

private:
	FString ActorNameSafe(AActor* Actor) const;
};