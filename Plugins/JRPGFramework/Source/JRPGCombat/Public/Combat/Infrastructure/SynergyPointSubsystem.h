#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/SP/SynergyPointTypes.h"
#include "SynergyPointSubsystem.generated.h"

class USynergyPointSettingsDataAsset;
class UBattleSessionSubsystem;

/**
 * - SP 저장/누적/Ready 판정의 단일 권위
 * - 다른 시스템은 Set 금지, SubmitGainEvent(이벤트)로만 요청
 * - 전투 중 감소 없음 / 체인 종료 or 전투 종료에서만 Reset
 * - BaseGain → RoleBonus(Outcome 기반) → TacticalBonus(배율/플랫)
 * - 제한: SPMaxGainPerSec, SameEventCooldownSec
 */
UCLASS()
class JRPGCOMBAT_API USynergyPointSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	// Events 
	FOnSynergyPointChanged OnSynergyPointChanged;
	FOnSynergyReadyChanged OnSynergyReadyChanged;
	FOnSynergyPointGained OnSynergyPointGained;

	// Settings (DA)
	UPROPERTY() TObjectPtr<USynergyPointSettingsDataAsset> SettingsAsset = nullptr;

	// State
	const FSynergyPointState& GetState() const { return State; }
	int32 GetSPForUI() const { return Settings.bOvercapAllowed ? FMath::Min(State.CurrentSP, State.SPCap) : State.CurrentSP; }

	// Submit 
	// InEvent: Instigator/Target/Role/Type/Outcome/bFromTacticalReservation/SourceTag만 채우고 넣으면 됨
	bool SubmitGainEvent(const FSPGainEvent& InEvent, FName ReasonTag);

	// Reset 
	void ResetForChainEnd();   // Chain 시스템이 호출
	void ResetForBattleEnd();  // BattleSession end에서 호출
	void Reset(FName ReasonTag);

	// ---- Outcome 기반 RoleBonus 입력(자동 생성) ----
	// Defender: Threat target change 기반
	void NotifyEnemyTargetChanged(AActor* Enemy, AActor* OldTarget, AActor* NewTarget);

	// Attacker: Groggy Stunned 전환 기반(“마지막 기여자” 보상)
	void NotifyVictimStunned(AActor* Victim);

	// UTickableWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USynergyPointSubsystem, STATGROUP_Tickables); }

private:
	FSynergyPointState State;
	FSynergyPointSettings Settings; // SettingsAsset 없으면 이 값 사용

	// 제한
	double GainWindowStartReal = 0.0;
	int32 GainWindowAccum = 0;

	// 동일 이벤트 쿨다운
	TMap<FName, double> LastEventRealTimeByKey;

	// Defender AggroHold 추적(Enemy->현재 타겟/유지시간)
	struct FAggroHoldTrack
	{
		TWeakObjectPtr<AActor> CurrentTarget;
		double HoldStartReal = 0.0;
		double LastHoldBonusReal = -1e9;
	};
	TMap<TWeakObjectPtr<AActor>, FAggroHoldTrack> AggroHoldByEnemy;

	// Attacker: 마지막 Break 기여자(Victim->Instigator)
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>> LastBreakInstigatorByVictim;

	// Attacker: DamageWindow (Instigator -> window)
	struct FDamageWindow
	{
		double WindowStartReal = 0.0;
		float AccumDamage = 0.f;
		double LastBonusReal = -1e9;
	};
	TMap<TWeakObjectPtr<AActor>, FDamageWindow> DamageWindowByInstigator;

	// battle active
	bool IsBattleActive() const;

	// compute pipeline
	int32 ComputeBaseGain(ESPEventType Type) const;
	int32 ComputeRoleBonus(FSPGainEvent& InOutEvent); // returns role bonus (fills maps as needed)
	int32 ApplyTacticalBonus(int32 RoleBonus, bool bFromTactical) const;

	bool PassSameEventCooldown(const FSPGainEvent& E, FName ReasonTag) ;
	FName MakeCooldownKey(const FSPGainEvent& E, FName ReasonTag) const;

	int32 ApplyPerSecondCap(int32 ProposedGain, double NowReal);
	void ApplyGainInternal(FSPGainEvent& Snapshot, int32 TotalGain, FName ReasonTag);

	// ready
	void UpdateReady();

	// helpers
	ECombatRole GetRoleOf(AActor* Actor) const;
};