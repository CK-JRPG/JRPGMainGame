#pragma once

#include "CoreMinimal.h"
#include "JRPGCoreApiTypes.h"
#include "ThreatTypes.generated.h"


/**
 * 위협도 이벤트 종류(확장용)
 * - 기본 구현은 AddThreat()만 써도 되고,
 * - 스킬/기본전투가 커지면 ReportThreatEvent()로 표준화해서 넣을 수 있음.
 */
UENUM()
enum class EThreatEventKind : uint8
{
	DamageTaken,     // (적 입장) 누가 나를 때렸는가
	HealDone,        // (적 입장) 누가 상대를 힐했는가(파티 힐 등)
	BuffDone,        // 버프/도발/디버프 등
	DirectThreat     // 수동 AddThreat 등
};

USTRUCT()
struct FThreatEvent
{
	GENERATED_BODY()

	UPROPERTY() EThreatEventKind Kind = EThreatEventKind::DirectThreat;
	UPROPERTY() TObjectPtr<AActor> Source = nullptr;      // 위협도를 유발한 대상(보통 플레이어)
	UPROPERTY() float BaseAmount = 0.f;                  // 원 위협도(곱셈 전)
	UPROPERTY() FName SourceTag = NAME_None;             // "Skill.Fireball", "HP.Damage" 등
};

USTRUCT()
struct FThreatEntryRuntime
{
	GENERATED_BODY()

	UPROPERTY() float Threat = 0.f;
	UPROPERTY() float LastUpdateRealTime = 0.f;
	UPROPERTY() int32 RecentHits = 0; // 히트 누적(선택)
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnThreatTargetChanged, AActor* /*Old*/, AActor* /*New*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnThreatTableChanged, AActor* /*Owner*/);
