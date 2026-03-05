#pragma once

#include "CoreMinimal.h"
#include "SynergyPointTypes.generated.h"

/**
 * 문서 8.1 Core Structs (SSOT)
 * - FSynergyPointState
 * - FSPGainEvent
 * + 구현 편의용(OutcomeValue 등) 확장 필드 포함(SSOT 위배 아님: 계산에 필요한 입력)
 */

UENUM()
enum class ECombatRole : uint8
{
	Defender,
	Attacker,
	Supporter,
	Unknown
};

UENUM()
enum class ESPEventType : uint8
{
	// Base gain (문서 5.1)
	Damage,      // 유효 피해
	Heal,        // 유효 힐
	Taunt,       // 도발/위협 성공
	Buff,        // 유효 버프 적용
	Debuff,      // 유효 디버프/상태이상 적용
	Cleanse,     // 정화/해제
	Break,       // 브레이크 기여(게이지 누적)

	// Role bonus discrete events (문서 5.2)
	DefenderAggroHold,
	DefenderAggroRescue,
	DefenderPartyProtect,

	AttackerStunTrigger,
	AttackerDamageWindow,

	SupporterCriticalHeal,
	SupporterBuffUptime
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

	// 누가/상대
	UPROPERTY() TObjectPtr<AActor> Instigator = nullptr; // 누가
	UPROPERTY() TObjectPtr<AActor> Target = nullptr;     // 상대/아군(상황에 따라)

	// 역할/이벤트 타입
	UPROPERTY() ECombatRole Role = ECombatRole::Unknown;
	UPROPERTY() ESPEventType Type = ESPEventType::Damage;

	// 계산 결과(SSOT)
	UPROPERTY() int32 BaseAmount = 0;
	UPROPERTY() int32 RoleBonusAmount = 0;
	UPROPERTY() int32 TacticalBonusAmount = 0;

	// 전술 예약 성공 여부(문서 5.3 / 11.2)
	UPROPERTY() bool bFromTacticalReservation = false;

	// 타임스탬프(RealTime)
	UPROPERTY() double TimestampReal = 0.0;

	// ---- 구현 입력 확장 필드(Outcome 기반 판정에 필요) ----
	// 예: Damage=피해량, Heal=회복량, Break=브레이크 누적량, AggroHold=유지 초
	UPROPERTY() float OutcomeValue = 0.f;

	// SupporterCriticalHeal 판정용(힐 직전 HP 비율)
	UPROPERTY() float TargetHPBeforeRatio = 1.f;

	// 디버그/텔레메트리 태그
	UPROPERTY() FName SourceTag = NAME_None;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSynergyPointChanged, int32 /*CurrentSP*/, int32 /*Delta*/, FName /*ReasonTag*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSynergyReadyChanged, bool /*bReady*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSynergyPointGained, const FSPGainEvent& /*Snapshot*/);