#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JRPGCoreApiTypes.h"
#include "Combat/Core/CombatDamageTypes.h"
#include "Combat/SP/SPTypes.h"
#include "SPEventRouterSubsystem.generated.h"

class USynergyPointSubsystem;

/**
 * SP Event Router (World SSOT)
 * - Damage/Heal/WallSlam/SlamGround 등 다양한 이벤트를 FJRPGSPGainEvent로 표준화
 * - SynergyPointSubsystem에 Apply
 */
UCLASS()
class JRPGCOMBAT_API USPEventRouterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	// 피해/회복 결과를 SP 이벤트로 변환
	void RouteDamageEvent(const FCombatDamageSpec& Spec, const FCombatDamageResult& Result, AActor* Victim);

private:
	USynergyPointSubsystem* GetSPSubsystem() const;
};
