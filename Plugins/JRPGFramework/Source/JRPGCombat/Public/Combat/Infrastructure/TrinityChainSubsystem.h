#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "Combat/Chain/ChainSettingsDataAsset.h"

#include "Combat/Chain/ChainTypes.h"
#include "TrinityChainSubsystem.generated.h"

class UChainSettingsDataAsset;
class UCombatTimeSubsystem;
class UCombatBattleSessionSubsystem;
class UCombatTacticalModeSubsystem;
class UCombatSynergyPointSubsystem;
class UJRPGSkillComponent;
class UJRPGSkillDataAsset;

UCLASS()
class JRPGCOMBAT_API UTrinityChainSubsystem :public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY()TObjectPtr<UChainSettingsDataAsset> SettingsAsset =nullptr;

	// events
	FOnChainStateChanged OnChainStateChanged;
	FOnChainStepPickChanged OnChainStepPickChanged;
	FOnChainTimerUpdated OnChainTimerUpdated;
	FOnChainStepExecuted OnChainStepExecuted;
	FOnChainEnded OnChainEnded;

	// AI hook
	FChainAutoPickSkillDelegate AutoPickSkillDelegate;

	// ---- State / Query ----
	EChainState GetState() const { return Ctx.State; }
	bool IsChainActive() const { return Ctx.State!= EChainState::Idle; }
	FChainToken GetToken() const { return Ctx.Token; }
	AActor* GetPrimaryTarget() const { return Ctx.PrimaryTarget.Get(); }
	int32 GetTPTotal() const { return Ctx.TPTotal; }

	float GetSelectionElapsedRealSec()const;
	float GetSelectionRemainingRealSec()const;
	float GetSelectionNormalized01()const;

	void GetStepViewsForUI(TArray<FChainStepView> &Out)const;

	// ---- API ----
	FJRPGOpResult TryStartChain();// uses session PrimaryTarget
	FJRPGOpResult TryStartChainWithTarget(AActor *PrimaryTarget);

	FJRPGOpResult SubmitStepPick(AActor *Caster,FName SkillId);// toggle/replace
	FJRPGOpResult ClearStepPick(AActor *Caster,FName ReasonTag);
	FJRPGOpResult ConfirmSelection();

	FJRPGOpResult AbortChain(FName ReasonTag);

	// Tickable
	virtual void Initialize(FSubsystemCollectionBase &Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(UTrinityChainSubsystem,STATGROUP_Tickables); }

private:
	FChainSettings Settings;
	FChainContext Ctx;

	// cached subsystems
	UCombatTimeSubsystem* GetTime() const;
	UCombatBattleSessionSubsystem* GetSession() const;
	UCombatTacticalModeSubsystem* GetTactical() const;
	UCombatSynergyPointSubsystem* GetSP() const;

	// internals
	void TransitionTo(EChainState NewState);

	bool GuardCanStart(AActor *PrimaryTarget, FJRPGReason &OutReason)const;

	void BeginSelection(AActor *PrimaryTarget);
	void AutoConfirmIfDeadline(double NowReal);

	void EnsurePartyActors();
	bool IsCasterPickable(AActor *Caster, FName &OutReasonTag) const;
	bool IsSkillEligibleForChain(AActor *Caster, FName SkillId, FName &OutReasonTag) const;

	const UJRPGSkillDataAsset* FindSkillAsset(AActor *Caster,FName SkillId) const;

	void BuildExecuteQueueIfNeeded();
	void ExecuteNextStep();
	void ExecuteFinisher();
	void EndChain(bool bAborted,FName ReasonTag);

	// enemy suppression + input lock
	void ApplyEntryGuards();
	void ReleaseEntryGuards();

	// chain-inside groggy(간단 버전: PrimaryTarget이 Stun tag를 새로 얻으면 bonus armed)
	void TickChainInsideGroggy();

	// TP
	int32 ComputeTPForSkill(const UJRPGSkillDataAsset *SkillAsset) const;
	float ComputeDamageScalarForStep() const;
	float ComputeFinisherDamageScalar() const;
};