#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "JRPGHUD.generated.h"

class UExplorationUIWidget;
class UCombatUIWidget;
class UTacticalUIWidget;
class UCombatHUDPresenter;
class UExplorationHUDPresenter;

UCLASS()
class JRPG_API AJRPGHUD : public AHUD
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
	// --- 프레젠터 (화면 흐름 및 데이터 중개자) ---
	UPROPERTY()
	TObjectPtr<UExplorationHUDPresenter> ExplorationPresenter;

	UPROPERTY()
	TObjectPtr<UCombatHUDPresenter> CombatPresenter;

	// --- 단일 위젯 (아직 프레젠터가 없는 요소) ---
	UPROPERTY()
	TObjectPtr<UTacticalUIWidget> TacticalWidget;

	void OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot);
	void OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot);

	// [콘솔 명령어]
public:
	UFUNCTION(Exec, Category = "JRPG|Test")
	void TestRegionName(const FString& RegionName);
	UFUNCTION(Exec, Category = "JRPG|Test")
	void TestPartyChat(const FString& Message);
};