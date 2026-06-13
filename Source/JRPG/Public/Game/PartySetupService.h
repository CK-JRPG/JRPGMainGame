#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PartySetupService.generated.h"

class UDataTable;

// 파티 초기화 브릿지 서브시스템
// DataTable -> SpawnEntry 변환 및 등록
// CharacterRuntimeSubsystem 스냅샷 초기화
// CombatControllerClass 설정 전달
// 필드 컴패니언 스폰 트리거

UCLASS()
class JRPG_API UPartySetupService : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	void InitializeCombatBridge(UDataTable* CharacterTable,
		TSubclassOf<APlayerController> CombatControllerClass,
		APawn* PlayerPawn
	);
};