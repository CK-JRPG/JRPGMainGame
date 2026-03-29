#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/StreamableManager.h"
#include "FieldCompanionSubsystem.generated.h"

class AJRPGCompanionPawn;
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

	// 전투 종료 시 컴패니언 복원
	void RestoreCompanions();

private:
	UPROPERTY()
	TMap<FName, TObjectPtr<AJRPGCompanionPawn>> SpawnedCompanionMap;

	TSharedPtr<FStreamableHandle> FieldPawnPreloadHandle;
};