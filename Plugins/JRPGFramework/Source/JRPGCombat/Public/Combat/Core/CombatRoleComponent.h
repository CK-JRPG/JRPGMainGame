#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/SP/JRPGSynergyPointTypes.h"
#include "CombatRoleComponent.generated.h"

/**
 * 플레이어/동료 캐릭터에 붙여서 역할을 SSOT로 제공
 * - Tactical Mode에서도 예약 스킬의 Role 판정 입력으로 그대로 씀
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UCombatRoleComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category="JRPG|Role")
	ECombatRole Role = ECombatRole::Unknown;

	ECombatRole GetRole() const { return Role; }
};