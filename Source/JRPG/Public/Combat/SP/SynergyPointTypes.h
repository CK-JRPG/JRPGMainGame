// Source/JRPGCombat/Public/Combat/SP/SynergyPointTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Core/RoleTypes.h"
#include "SynergyPointTypes.generated.h"

UENUM()
enum class ESPEventType : uint8
{
	None,
	Damage,
	Heal,
	Taunt,
	Cleanse,
	Buff,
	Debuff,
	Break,
	Protect
};

USTRUCT()
struct FSynergyPointState
{
	GENERATED_BODY()

	UPROPERTY() int32 CurrentSP = 0;
	UPROPERTY() int32 SPCap = 100;
	UPROPERTY() bool bChainReady = false;
	UPROPERTY() double LastGainRealTime = 0.0;
};

USTRUCT()
struct FSPGainEvent
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Instigator;
	UPROPERTY() TWeakObjectPtr<AActor> Target;

	UPROPERTY() EPartyRole Role = EPartyRole::Attacker;
	UPROPERTY() ESPEventType Type = ESPEventType::None;

	UPROPERTY() int32 BaseAmount = 0;
	UPROPERTY() int32 RoleBonusAmount = 0;
	UPROPERTY() int32 TacticalBonusAmount = 0;
	UPROPERTY() int32 FinalGrantedAmount = 0;

	UPROPERTY() bool bFromTacticalReservation = false;
	UPROPERTY() double TimestampReal = 0.0;

	UPROPERTY() FName ReasonTag = NAME_None;
};

USTRUCT()
struct FSynergyPointTuning
{
	GENERATED_BODY()

	// 문서상 상세 수치는 TBD이므로, 여기 값은 “기본 안전값”이다.
	UPROPERTY(EditAnywhere) int32 SPCap = 100;
	UPROPERTY(EditAnywhere) int32 SPMaxGainPerSec = 25;
	UPROPERTY(EditAnywhere) float SameEventCooldownSec = 1.5f;

	UPROPERTY(EditAnywhere) float TacticalRoleBonusMultiplier = 1.25f;
	UPROPERTY(EditAnywhere) int32 TacticalFlatBonus = 0;
	UPROPERTY(EditAnywhere) bool bOvercapAllowed = true;

	// Defender
	UPROPERTY(EditAnywhere) int32 DefenderAggroHoldBonus = 6;
	UPROPERTY(EditAnywhere) int32 DefenderAggroRescueBonus = 12;
	UPROPERTY(EditAnywhere) int32 DefenderPartyProtectBonus = 8;

	// Attacker
	UPROPERTY(EditAnywhere) float AttackerBreakContributionCoeff = 0.20f;
	UPROPERTY(EditAnywhere) int32 AttackerStunTriggerBonus = 12;
	UPROPERTY(EditAnywhere) float AttackerDamageWindowSec = 1.0f;
	UPROPERTY(EditAnywhere) float AttackerDamageWindowThreshold = 120.f;
	UPROPERTY(EditAnywhere) int32 AttackerDamageWindowBonus = 8;

	// Supporter
	UPROPERTY(EditAnywhere) float SupporterCriticalHealThreshold = 0.30f;
	UPROPERTY(EditAnywhere) int32 SupporterCriticalHealBonus = 10;
	UPROPERTY(EditAnywhere) int32 SupporterCleanseBonus = 10;
	UPROPERTY(EditAnywhere) int32 SupporterBuffUptimeBonus = 4;
};

DECLARE_MULTICAST_DELEGATE_FourParams(FOnSynergyPointChanged, int32/*CurrentSP*/, int32/*Delta*/, ESPEventType/*Type*/, FName/*ReasonTag*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSynergyReadyChanged,bool/*bReady*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSynergyGainApplied,const FSPGainEvent& /*Event*/);