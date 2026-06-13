#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CompanionPawnController.generated.h"

UENUM(BlueprintType)
enum class ECompanionAdventureState : uint8
{
	None			UMETA(DisplayName = "초기화 전"),
	Idle			UMETA(DisplayName = "대기 (산개 및 휴식)"),
	FollowLeader	UMETA(DisplayName = "리더 추적"),
	TeleportCatchUp UMETA(DisplayName = "순간이동 합류")
};

/**
 * 필드 동료 AI 컨트롤러 (NavMesh 미사용, 순수 FSM)
 * - Idle: 리더 근처 대기, 거리 초과 시 FollowLeader 전환
 * - FollowLeader: 대형 위치를 향해 직접 이동 (CharacterMovementComponent)
 * - TeleportCatchUp: 카메라 밖이면 즉시 텔레포트, 안이면 FollowLeader로 빠르게 추적
 */
UCLASS()
class JRPG_API ACompanionPawnController : public AAIController
{
	GENERATED_BODY()

public:
	ACompanionPawnController();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "JRPG|Companion")
	void SetLeaderActor(AActor* InLeader);

	UFUNCTION(BlueprintCallable, Category = "JRPG|Companion")
	void SetPartyIndex(int32 InIndex);

	UFUNCTION(BlueprintPure, Category = "JRPG|Companion")
	int32 GetPartyIndex() const { return PartyIndex; }

	UFUNCTION(BlueprintCallable, Category = "JRPG|Companion")
	void SetAdventureState(ECompanionAdventureState NewState);

protected:
	virtual void OnPossess(APawn* InPawn) override;

	// --------------------------------------------------------
	// FSM 코어 로직
	// --------------------------------------------------------
	void OnEnterState(ECompanionAdventureState State);
	void OnExitState(ECompanionAdventureState State);

	void UpdateIdle(float DeltaTime);
	void UpdateFollowLeader(float DeltaTime);
	void UpdateTeleportCatchUp(float DeltaTime);

	// --------------------------------------------------------
	// NavMesh 미사용 직접 이동
	// --------------------------------------------------------
	void MoveDirectlyToward(const FVector& Destination, float DeltaTime);

	// --------------------------------------------------------
	// 유틸리티
	// --------------------------------------------------------
	void SyncMovementSpeedWithLeader();
	FVector GetFormationLocation() const;
	bool IsInCameraFrustum() const;

	// --------------------------------------------------------
	// 상태 및 수치 데이터
	// --------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JRPG|Companion|State")
	ECompanionAdventureState CurrentState = ECompanionAdventureState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JRPG|Companion|Data")
	TObjectPtr<AActor> LeaderActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JRPG|Companion|Data")
	int32 PartyIndex = 1;

	// 거리 파라미터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JRPG|Companion|Distances")
	float FollowStartRadius = 400.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JRPG|Companion|Distances")
	float FollowStopRadius = 200.0f;  

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JRPG|Companion|Distances")
	float TeleportRadius = 2500.0f;

	// 텔레포트 상태에서 카메라에 보일 때 사용하는 빠른 추적 제한 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JRPG|Companion|Distances")
	float TeleportCatchUpTimeoutSeconds = 3.0f;

	// 텔레포트 추적 시 속도 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JRPG|Companion|Distances")
	float CatchUpSpeedMultiplier = 2.0f;

private:
	float CatchUpTimer = 0.f;
};
