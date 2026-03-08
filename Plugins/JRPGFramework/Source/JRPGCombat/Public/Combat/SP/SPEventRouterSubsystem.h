#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/Core/CombatDamageTypes.h"
#include "Combat/SP/JRPGSynergyPointTypes.h"
#include "SPEventRouterSubsystem.generated.h"

/**
 * 문서 11.1: “다른 시스템들이 결과 이벤트를 발행하고 SP 시스템이 소비”
 * - 라우터는 결과를 FSPGainEvent로 표준화해서 SynergyPointSubsystem에 Submit
 */
UCLASS()
class JRPGCOMBAT_API USPEventRouterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
  
public:
	// 기본전투 HPComponent가 호출
	void RouteDamageOrHeal(const FCombatDamageSpec& Spec, const FCombatDamageResult& Result, AActor* Victim);

	// 스킬/상태 시스템에서 선택적으로 호출(정화/버프/디버프)
	void RouteStatusApplied(AActor* Instigator, AActor* Target, FName StatusId, bool bDebuff, bool bFromTacticalReservation, FName SourceTag);
	void RouteStatusCleansed(AActor* Instigator, AActor* Target, FName RemovedStatusId, bool bFromTacticalReservation, FName SourceTag);

	// 버프 유지 보너스(서폿 uptime)
	void RouteSupporterBuffUptime(AActor* Supporter, AActor* Target, float UptimeSec, bool bFromTacticalReservation, FName SourceTag);
};
