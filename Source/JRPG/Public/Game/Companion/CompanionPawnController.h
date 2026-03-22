#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CompanionPawnController.generated.h"

class UCrowdFollowingComponent;
class ULocomotionComponent;

UENUM(BlueprintType)
enum class ECompanionAdventureState : uint8
{
	None			UMETA(DisplayName = "초기화 전"),
	Idle			UMETA(DisplayName = "대기 (산개 및 휴식)"),
	FollowLeader	UMETA(DisplayName = "리더 추적"),
	TeleportCatchUp UMETA(DisplayName = "순간이동 합류")
};

UCLASS()
class JRPG_API ACompanionPawnController : public AAIController
{
	GENERATED_BODY()

public:
	ACompanionPawnController(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "JRPG|Companion")
	void SetLeaderActor(AActor* InLeader);

	// 파티 대형에서의 인덱스 (0: 리더, 1: 좌측 후방, 2: 우측 후방 등)
	UFUNCTION(BlueprintCallable, Category = "JRPG|Companion")
	void SetPartyIndex(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "JRPG|Companion")
	void SetAdventureState(ECompanionAdventureState NewState);

protected:
	virtual void OnPossess(APawn* InPawn) override;

	// --------------------------------------------------------
	// FSM 코어 로직 (Enter / Update / Exit 패턴 도입)
	// --------------------------------------------------------
	void OnEnterState(ECompanionAdventureState State);
	void OnExitState(ECompanionAdventureState State);

	void UpdateIdle(float DeltaTime);
	void UpdateFollowLeader(float DeltaTime);

	// --------------------------------------------------------
	// 유틸리티 및 확장 기능
	// --------------------------------------------------------
	// 리더의 이동 속도(걷기/뛰기)를 폰에 동기화
	void SyncMovementSpeedWithLeader();

	// 파티 인덱스를 기반으로 한 체계적인 대형 위치 계산
	FVector GetFormationLocation() const;

	// 화면에 보이고 있는지 체크 (텔레포트 방지용)
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
};