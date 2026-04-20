// Source/JRPGCombat/Public/Combat/AI/CombatAIContext.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Combat/AI/CombatAIPresetAsset.h"
#include "Combat/AI/CombatAIInterfaces.h"
#include "CombatAIContext.generated.h"

class UCombatAIPresetAsset;
class USkillComponent;
class UHPComponent;
class UThreatComponent;

UCLASS()
class JRPG_API UCombatAIContext : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AActor*InOwner,EJRPGPartyRole InRole,UCombatAIPresetAsset *InPresetAsset);
	void Refresh();

	// ---- World/Owner
	UPROPERTY() TWeakObjectPtr<AActor> Owner;
	UPROPERTY() TWeakObjectPtr<APawn> OwnerPawn;
	UPROPERTY() TWeakObjectPtr<AController> OwnerController;

	UPROPERTY() EJRPGPartyRole Role = EJRPGPartyRole::Attacker;
	UPROPERTY() TWeakObjectPtr<UCombatAIPresetAsset> PresetAsset;

	// ---- Components
	UPROPERTY() TWeakObjectPtr<USkillComponent> SkillComp;
	UPROPERTY() TWeakObjectPtr<UHPComponent> HPComp;
	UPROPERTY() TWeakObjectPtr<UThreatComponent> ThreatComp;

	// ---- Targets/Party
	UPROPERTY() TWeakObjectPtr<AActor> PrimaryTarget;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> PartyMembers;

	// ---- Flags
	UPROPERTY() bool bSessionActive = false;// BattleSessionSubsystem Phase==Active 등으로 채움
	UPROPERTY() bool bInTactical = false;
	UPROPERTY() bool bInChainSequence = false;

	UPROPERTY() bool bSelfIsDead = false;
	UPROPERTY() float SelfHp01 = 1.f;

	// Ally danger snapshot
	UPROPERTY() bool bAnyAllyCritical = false;
	UPROPERTY() TWeakObjectPtr<AActor> AllyCriticalTarget;
	UPROPERTY() bool bAnyAllyHasCC = false;
	UPROPERTY() TWeakObjectPtr<AActor> AllyCC_Target;

	// Target groggy snapshot
	UPROPERTY() EJRPGGroggyPhase TargetGroggyPhase = EJRPGGroggyPhase::Normal;
	UPROPERTY() float TargetBreakRatio01 = 0.f;

	// SP snapshot
	UPROPERTY() int32 CurrentSP = 0;
	UPROPERTY() int32 SPCap = 0;
	UPROPERTY() bool bChainReady = false;
	UPROPERTY() FCombatSPSettingsView SPSettings;

private:
	void RefreshSubsystemFlags();
	void RefreshPartySnapshot();
	void RefreshTargetSnapshot();
	void RefreshSP();

	// Helpers: 인터페이스 구현 컴포넌트를 찾아서 읽는다.
	static bool TryReadGroggyFromActor(AActor *Actor, EJRPGGroggyPhase &OutPhase, float &OutBreakRatio01);
	bool TryReadSPFromWorld(UWorld *World, int32 &OutCurrent, int32 &OutCap, bool &OutReady, FCombatSPSettingsView &OutSettings);
	bool TryReadChainActiveFromWorld(UWorld *World, bool &bOutChainActive);
	
	UPROPERTY(Transient) TWeakObjectPtr<UObject> CachedSPProviderObject;
	UPROPERTY(Transient) TWeakObjectPtr<UObject> CachedChainProviderObject;
	double NextProviderRescanAt = 0.0;
	static constexpr double ProviderRescanIntervalSec = 1.0;
};