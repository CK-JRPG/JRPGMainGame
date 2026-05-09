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
	
	UPROPERTY() FRotator ControlRotation = FRotator::ZeroRotator;
	UPROPERTY() FVector  FLocation = FVector::ZeroVector;
	UPROPERTY() FRotator FRotator = FRotator::ZeroRotator;
	UPROPERTY() float	 ArmLength = 550.0f;
	UPROPERTY() bool bHasControlRotation = false;
	
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

	// 인카운터 진입 직전 또는 전투 종료 직전 현재 카메라 상태 저장
	void SaveFieldSnapshot(AActor* OverrideTarget = nullptr);
	
	// 전투 종료 후
	void RestoreFieldSnapshot();
	
	// 저장된 필드 컨트롤 회전 조회
	bool GetSavedFieldControlRotation(FRotator& OutControlRotation) const;

	// 카메라 줌 인/아웃 관련
	void AdjustZoom(float NormalizedDelta);
	void ResetZoom();
	
	// 적 락온
	void LockOnEnemy();
	void CycleLockOnEnemy(int32 Direction);
	void ClearLockOn();
	bool IsLockedOn() const { return bLockedOn; }
	AActor* GetLockedOnEnemy() const { return LockedOnEnemy.Get(); }

	// Getter/Setter
	ACameraRigActor* GetCameraRig() const { return CameraRig.Get(); }
	
protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
private:
	UPROPERTY() TWeakObjectPtr<ACameraRigActor> CameraRig;
	UPROPERTY() FCameraFieldSnapshot FieldSnapshot;

	// 적 락온 상태
	bool bLockedOn = false;
	int32 LockedOnEnemyIndex = INDEX_NONE;
	UPROPERTY() TWeakObjectPtr<AActor> LockedOnEnemy;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> CachedEnemies;
	
	void RefreshEnemyList();


	// BattleSessionSubsystem 델리게이트 콜백
	void OnBattleStarted(const struct FBattleSessionSnapshot& Snapshot);
	void OnBattleEnded(const struct FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);
	
	// BattleSessionSubsystem 빙의 전환 델리게이트 콜백
	void OnCharacterPossessed(AActor* NewCharacter);
};
