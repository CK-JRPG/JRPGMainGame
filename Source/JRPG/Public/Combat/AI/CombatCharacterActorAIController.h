#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CombatCharacterActorAIController.generated.h"

/**
 * 전투 중 플레이어가 조작하지 않는 아군 CombatCharacterActor에 할당되는 AI 컨트롤러
 * 실제 전투 AI 로직은 CombatPartyAIComponent가 처리함
 * 이 컨트롤러는 빙의 상태를 관리하는 역할만 담당한다
 */
UCLASS()
class JRPG_API ACombatCharacterActorAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACombatCharacterActorAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
};
