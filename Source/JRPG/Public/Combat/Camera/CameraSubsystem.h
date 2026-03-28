#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CameraSubsystem.generated.h"

class ACameraRigActor;
enum class EBattleEndReason : uint8;

// 스냅샷 - 인카운터 진입 전 필드 카메라 상태 보존용
USTRUCT()
struct FCameraFieldSnapshot
{
	GENERATED_BODY()
	
	UPROPERTY() FVector  FLocation = FVector::ZeroVector;
	UPROPERTY() FRotator FRotator = FRotator::ZeroRotator;
	UPROPERTY() float	 ArmLength = 400.0f;
	
	UPROPERTY() TWeakObjectPtr<AActor> Target;
	
	bool IsValid() const { return Target.IsValid(); }
};

/**
 * 
 */
UCLASS()
class JRPG_API UCameraSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// 타겟 전환
	void SetTarget(AActor* NewTarget);
	
	// 타겟 전환 (끊김 없는 보간 전환)
	void SetTargetSmooth(AActor* NewTarget);
	
	// 인카운터 진입 직전
	void SaveFieldSnapshot();
	
	
	// 전투 종료 후
	void RestoreFieldSnapshot();
	
	ACameraRigActor* GetCameraRig() const { return CameraRig.Get(); }
	
protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
private:
	UPROPERTY() TWeakObjectPtr<ACameraRigActor> CameraRig;
	UPROPERTY() FCameraFieldSnapshot FieldSnapshot;
	
	// BattleSessionSubsystem 델리게이트 콜백
	void OnBattleEnded(const struct FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);
	
	// BattleSessionSubsystem 빙의 전환 델리게이트 콜백
	void OnCharacterPossessed(AActor* NewCharacter);
};
