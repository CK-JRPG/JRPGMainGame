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

	// 전환 중 여부 반환 (인카운터 트리거에서 사용)
	bool IsTransitioning() const { return bIsTransitioning; }


private:
	// 승리/패배 분기 처리
	void HandleVictoryTransition();
	void HandleDefeatTransition();

	// 패배 비동기 흐름
	void OnDefeatFadeOutComplete();
	void OnDefeatFadeInComplete();

	// 승리 후 점진적 HP 회복
	void StartPostBattleRecovery();
	void TickPostBattleRecovery();

	// 공통 전환 서브 함수
	void PerformTransition(bool bUseLeaderPosition);
	void ReturnPossessionToLeader();
	void SaveLeaderTransformAndSync(FVector& OutLocation, FRotator& OutRotation, bool& bOutValid);
	void HandleDefeatRecovery();
	void RestoreFieldController(APlayerController*& OutControllerToRestore, APlayerController*& OutCombatPCToDestroy);
	void RestoreFieldPawn(APlayerController* ControllerToRestore, const FVector& LeaderLocation, const FRotator& LeaderRotation, bool bHasLeaderTransform);
	void ResetTransitionState();

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

	// 패배 처리용 타이머
	FTimerHandle DefeatFadeOutTimerHandle;
	FTimerHandle DefeatFadeInTimerHandle;

	// 전투 -> 필드 전환 중 인카운터 재발동 방지 플래그
	bool bIsTransitioning = false;
	FTimerHandle EncounterImmuneTimerHandle;
	
	// 승리 후 회복용 타이머
	FTimerHandle PostBattleRecoveryTimerHandle;
	static constexpr float PostBattleRecoveryInterval = 1.0f;
	static constexpr float PostBattleRecoveryRatio = 0.05f;
	
	// 패배 전환 시 저장되는 필드 컨트롤러 (비동기 흐름용)
	UPROPERTY()
	TObjectPtr<APlayerController> DefeatRestoredController;
};