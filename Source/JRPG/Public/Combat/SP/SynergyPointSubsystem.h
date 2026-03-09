// Source/JRPGCombat/Public/Combat/SP/SynergyPointSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/SP/SynergyPointTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Chain/ChainAttackTypes.h"

#include "SynergyPointSubsystem.generated.h"

UCLASS()
class JRPG_API USynergyPointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnSynergyPointChanged OnSynergyPointChanged;
	FOnSynergyReadyChanged OnSynergyReadyChanged;
	FOnSynergyGainApplied OnSynergyGainApplied;


	const FJRPGSynergyPointState& GetState() const	 { return State; } 
	FSynergyPointTuning& GetMutableTuning()		 { return Tuning; }
	const FSynergyPointTuning& GetTuning() const { return Tuning; }

	bool IsChainReady()const {return State.bChainReady; }

	void ResetForBattleEnd(FName ReasonTag);
	void ResetForChainEnd(FName ReasonTag);

	void ReportDamage(AActor *Instigator, AActor *Target,
		float DamageAmount, bool bFromTacticalReservation,FName ReasonTag);

	void ReportHeal(AActor *Instigator, AActor *Target,
		float HealAmount, float TargetHPRatioBefore, bool bFromTacticalReservation, FName ReasonTag);

	void ReportBreak(AActor *Instigator, AActor *Target,
		float BreakAmount, bool bTriggeredStun, bool bFromTacticalReservation, FName ReasonTag);

	void ReportBuff(AActor *Instigator, AActor *Target, FName BuffId,
		bool bFromTacticalReservation, FName ReasonTag);

	void ReportDebuff(AActor *Instigator, AActor *Target, FName DebuffId,
		bool bFromTacticalReservation, FName ReasonTag);

	void ReportCleanse(AActor *Instigator, AActor *Target, 
		int32 RemovedCount, bool bRemovedCriticalCC, bool bFromTacticalReservation, FName ReasonTag);

	void ReportThreatOutcome(AActor*Instigator, AActor*EnemyOwner,
		float ThreatDelta, bool bBecameTopThreat, bool bRescuedAlly, bool bFromTacticalReservation, FName ReasonTag);

	void ReportAggroHold(AActor *Defender, AActor *EnemyOwner,
		bool bFromTacticalReservation, FName ReasonTag);

	void ReportPartyProtect(AActor *Defender, AActor *ProtectedTarget,
		bool bFromTacticalReservation, FName ReasonTag);

protected:
	virtual void OnWorldBeginPlay(UWorld &InWorld) override;

private:
	UPROPERTY() 
	FJRPGSynergyPointState State;
	
	UPROPERTY(EditAnywhere)
	FSynergyPointTuning Tuning;

	struct FRecentGainSlice
	{
		double TimeReal = 0.0;
		int32 Amount = 0;
	};

	struct FDamageWindowRuntime
	{
		double WindowStartReal = 0.0;
		float AccDamage = 0.f;
	};

	TArray<FRecentGainSlice> RecentGainSlices;
	TMap<FString, double> LastSameEventRealByKey;
	TMap<TWeakObjectPtr<AActor>, FDamageWindowRuntime> DamageWindows;

	bool bSubscribed = false;

	bool CanAcceptGain() const;
	void TryBindExternalEvents();

	EJRPGPartyRole ResolveRoleForActor(AActor *Actor) const;
	bool IsAlly(AActor *A, AActor *B) const;
	bool IsEnemy(AActor *A, AActor *B) const;

	int32 ComputeTacticalBonus(int32 RoleBonus,bool bFromTacticalReservation) const;
	bool PassesSameEventCooldown(const FString &EventKey, double Now);
	int32 ConsumePerSecondBudget(int32 ProposedAmount, double Now);

	void ApplyGainEvent(FJRPGSPGainEvent &Event);
	void UpdateReadyState();

	void HandleBattleEnded(const FBattleSessionSnapshot &Snapshot,EBattleEndReason Reason);
	void HandleChainEnded(const FChainAttackSnapshot &Snapshot);
};