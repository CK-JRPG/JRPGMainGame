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

private:
	// OnBattleEnded 서브 함수
	void ReturnPossessionToLeader();
	void SaveLeaderTransformAndSync(FVector& OutLocation, FRotator& OutRotation, bool& bOutValid);
	void HandleDefeatRecovery(EBattleEndReason Reason);
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
};