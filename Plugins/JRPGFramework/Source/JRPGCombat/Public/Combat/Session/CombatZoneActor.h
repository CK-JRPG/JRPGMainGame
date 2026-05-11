#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatZoneActor.generated.h"

class USphereComponent;

/**
 * 전투 구역(Clamp) 액터
 * - ZoneBounds(회전/스케일 포함) 로컬 공간에서 캐릭터 위치를 Clamp
 * - 캐릭터 캡슐 반지름/반높이를 고려해서 "캡슐 전체"가 구역 안에 있도록 보정
 */
UCLASS()
class JRPGCOMBAT_API ACombatZoneActor : public AActor
{
	GENERATED_BODY()
	
public:
	ACombatZoneActor();
	
	/** 캐릭터(캡슐) 위치를 Zone 내부로 Clamp */
	UFUNCTION(BlueprintCallable)
	FVector ClampCharacterLocation(const FVector& WorldLocation, float CapsuleRadius, float CapsuleHalfHeight) const;
	
	/** Zone 안에 들어와 있는지(캡슐 고려) */
	UFUNCTION(BlueprintCallable)
	bool IsCharacterInside(const FVector& WorldLocation, float CapsuleRadius, float CapsuleHalfHeight) const;

	UFUNCTION(BlueprintCallable)
	void SetZoneRadius(float InRadius);

	UFUNCTION(BlueprintCallable)
	void SetZoneHalfHeight(float InHalfHeight);

	USphereComponent* GetZoneBounds() const { return ZoneBounds; }
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> ZoneBounds;
	
	UPROPERTY(EditAnywhere, Category = "JRPG|CombatZone", meta = (ClampMin = "0.0"))
	float ZoneHalfHeight = 300.0f;

	/** 디버그 표시 */
	UPROPERTY(EditAnywhere, Category="JRPG|CombatZone")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, Category="JRPG|CombatZone", meta=(ClampMin="0.0"))
	float DebugDrawThickness = 2.0f;

	virtual void Tick(float DeltaSeconds) override;
};
