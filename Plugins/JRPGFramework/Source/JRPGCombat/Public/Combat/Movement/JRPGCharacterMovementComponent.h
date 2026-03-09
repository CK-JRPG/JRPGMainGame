#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JRPGCharacterMovementComponent.generated.h"

class ACombatZoneActor;

/**
 * 캐릭터 이동 결과를 전투 구역(CombatZone)에 맞춰 Clamp하는 커스텀 MovementComponent
 * - CurrentCombatZone이 설정되어 있을 때만 Clamp 수행
 * - Locomotion/CombatMotion과 독립적으로 "최종 위치"를 보정하는 안전장치 역할
 */
UCLASS()
class JRPGCOMBAT_API UJRPGCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	void SetCurrentCombatZone(ACombatZoneActor* InZone);
	ACombatZoneActor* GetCurrentCombatZone() const { return CurrentCombatZone.Get(); }

	/** 전투가 아닐 때 Clamp를 끄고 싶으면 false로 */
	void SetZoneClampEnabled(bool bEnabled) { bZoneClampEnabled = bEnabled; }
	bool IsZoneClampEnabled() const { return bZoneClampEnabled; }

protected:
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ACombatZoneActor> CurrentCombatZone;

	UPROPERTY(EditAnywhere, Category="JRPG|Clamp")
	bool bZoneClampEnabled = true;

	/** Clamp 시 순간 이동으로 보정(단일플레이 기준 가장 안정) */
	UPROPERTY(EditAnywhere, Category="JRPG|Clamp")
	bool bTeleportClamp = true;
};
