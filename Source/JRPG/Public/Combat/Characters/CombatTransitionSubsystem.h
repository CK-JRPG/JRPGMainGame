#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "CombatTransitionSubsystem.generated.h"

class ACombatPlayerController;
class ACombatCharacterActor;


// 전투 전환 및 동기화 서브시스템
// 전투 모드 진입 (EnterCombatMode)
// 전투 종료 복원 (OnBattleEnded)
// 전투 중 빙의 전환 (OnPartyMemberChanged)
// 이동 상태 동기화 (양방향)


UCLASS()
class JRPG_API UCombatTransitionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	// 전투 모드 시작시 진입함 - 필드 폰 숨기고 CombatCharacterActor에 빙의 
	void EnterCombatMode(APlayerController* PC, const FName& LeaderCharacterID);

	//전투 종료 시 필드 모드 복원 
	void OnBattleEnded(EBattleEndReason Reason);

	//전투 중 조작 캐릭터 전환
	void OnPartyMemberChanged(const FName& NewCharacterID);

	void SetOriginalPlayerCharacterID(const FName& CharacterID);
	FName GetCurrentPlayerCharacterID() const { return CurrentPlayerCharacterID; }

	void SetCombatControllerClass(TSubclassOf<APlayerController> InClass);

	// 허브 위치 등록/해제 (패배 시 가장 가까운 허브로 이동)
	void RegisterHubLocation(const FVector& Location);
	void UnregisterHubLocation(const FVector& Location);

private:
	// OnBattleEnded 서브 함수
	void ReturnPossessionToLeader();
	void SaveLeaderTransformAndSync(FVector& OutLocation, FRotator& OutRotation, bool& bOutValid);
	void HandleDefeatRecovery(EBattleEndReason Reason);
	void RestoreFieldController(APlayerController*& OutControllerToRestore, APlayerController*& OutCombatPCToDestroy);
	void RestoreFieldPawn(APlayerController* ControllerToRestore, const FVector& LeaderLocation, const FRotator& LeaderRotation, bool bHasLeaderTransform);
	void ResetTransitionState();

	// 전환 처리 (공통 로직)
	void PerformTransition();

	// 비동기 페이드 처리 (패배 전용)
	void OnFadeOutComplete();
	void OnFadeInComplete();
	void StartScreenFade(float FromAlpha, float ToAlpha, float Duration);

	// 허브 위치 검색
	FVector FindNearestHub(const FVector& FromLocation) const;

	// 전투 후 점차 HP 회복
	void StartPostBattleRecovery();
	void StopPostBattleRecovery();
	void TickPostBattleRecovery();

	//이동 상태 동기화
	void SyncMovementStateToLeader(APawn* FieldPawn, ACombatCharacterActor* LeaderActor);
	void SyncMovementStateToFieldPawn(ACombatCharacterActor* LeaderActor, APawn* FieldPawn);

	// 배틀세션 델리게이트 핸들러
	void HandleBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);

private:
	UPROPERTY()
	TObjectPtr<APlayerController> CombatPlayerController;

	UPROPERTY()
	TSubclassOf<APlayerController> CombatControllerClass;

	UPROPERTY()
	TObjectPtr<APlayerController> CachedFieldController;

	UPROPERTY()
	TObjectPtr<APawn> CachedFieldPawn;

	FName OriginalPlayerCharacterID;
	FName CurrentPlayerCharacterID;

	// 전투 진입 전 위치 (승리 시 복귀용)
	FVector CachedPreBattleLocation = FVector::ZeroVector;
	FRotator CachedPreBattleRotation = FRotator::ZeroRotator;
	bool bHasPreBattleTransform = false;

	// 비동기 전환용 상태
	EBattleEndReason PendingEndReason = EBattleEndReason::Aborted;
	FTimerHandle FadeOutTimerHandle;
	FTimerHandle FadeInTimerHandle;
	FTimerHandle PostBattleRecoveryTimerHandle;

	// 허브 위치 목록 (패배 시 가장 가까운 허브로 이동)
	TArray<FVector> HubLocations;

	// 페이드 및 전투 후 HP 회복 설정
	static constexpr float FadeDuration = 2.0f;
	static constexpr float PostBattleRecoveryInterval = 1.0f;
	static constexpr float PostBattleRecoveryRatio = 0.05f;
};