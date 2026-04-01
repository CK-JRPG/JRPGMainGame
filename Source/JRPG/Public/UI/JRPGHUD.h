 #pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "JRPGHUD.generated.h"

class UExplorationUIWidget;
class UCombatUIWidget;
class UTacticalUIWidget;

UCLASS()
class JRPG_API AJRPGHUD : public AHUD  // <--- JRPG_API 로 변경됨
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UExplorationUIWidget> ExplorationWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UCombatUIWidget> CombatWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UTacticalUIWidget> TacticalWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UExplorationUIWidget> ExplorationWidget;

	UPROPERTY()
	TObjectPtr<UCombatUIWidget> CombatWidget;
	
	UPROPERTY() 
	TObjectPtr<UTacticalUIWidget> TacticalWidget;

	void OnBattleStarted(const FBattleSessionSnapshot& Snapshot);
	void OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);
	
	void OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot);
	void OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot);
	
	// 실제 UI를 스위칭하는 내부 함수
	void SwitchToCombatUI();
	void SwitchToExplorationUI();
};