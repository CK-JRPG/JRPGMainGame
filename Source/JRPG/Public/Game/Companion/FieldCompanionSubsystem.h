#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/StreamableManager.h"
#include "FieldCompanionSubsystem.generated.h"

class AJRPGCompanionPawn;
class AActor;
class ACompanionPawnController;
struct FCharacterSpawnEntry;


// 필드 컴패니언 폰 라이프사이클 관리 서브시스템
// CompanionPawn 비동기 스폰/파괴
// 스폰된 컴패니언 조회
// 전투 진입 시 숨기기 / 전투 종료 시 복원

UCLASS()
class JRPG_API UFieldCompanionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//SpawnEntryMap 참조로 필드 컴패니언 비동기 스폰
	void SpawnFieldCompanions(
		const FVector& LeaderLocation,
		const FName& LeaderCharacterID,
		const TMap<FName, FCharacterSpawnEntry>& SpawnEntryMap
	);

	//스폰된 모든 필드 컴패니언 파괴
	void DespawnFieldCompanions();

	// 스폰된 컴패니언 배열 반환 
	UFUNCTION()
	TArray<AJRPGCompanionPawn*> GetSpawnedCompanions() const;

	// 전투 진입 시 컴패니언 숨기기 + AI 제거
	void HideCompanions();

	// 전투 종료 시 컴패니언 복원 (제자리 - 승리 시)
	void RestoreCompanions(AActor* LeaderActor = nullptr);

	// 전투 종료 시 컴패니언 복원 (저장된 위치로 - 패배 시)
	void RestoreCompanionsToSavedLocations(AActor* LeaderActor = nullptr);
	
	// 현재 컴패니언 위치를 스냅샷 저장 (허브 상호작용 시)
	void SaveCompanionLocations();
	
	// 저장된 컴페니언 위치가 있는지 확인
	bool HasSavedCompanionLocations() const;
	
private:
	FTransform MakeFormationTransform(const AActor* LeaderActor, int32 CompanionIndex) const;
	bool IsRestoreLocationUsable(const FVector& Location, const AActor* LeaderActor) const;
	void MoveToFormationIfNeeded(AJRPGCompanionPawn* Companion, const AActor* LeaderActor, int32 CompanionIndex, FName ReasonTag);
	void RestoreCompanion(AJRPGCompanionPawn* Companion, AActor* LeaderActor, int32 CompanionIndex);

	static constexpr float RestoreMaxDistanceFromLeader = 1800.0f;
	static constexpr float RestoreFormationDistance = 200.0f;

	UPROPERTY()
	TMap<FName, TObjectPtr<AJRPGCompanionPawn>> SpawnedCompanionMap;

	// 허브 상호작용 시 저장된 컴패니언 위치
	TMap<FName, FVector> SavedCompanionLocations;
	
	TSharedPtr<FStreamableHandle> FieldPawnPreloadHandle;
};
