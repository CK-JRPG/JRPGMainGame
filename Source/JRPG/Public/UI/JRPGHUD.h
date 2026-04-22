#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/MainMenu/MainMenuUIWidget.h"
#include "JRPGHUD.generated.h"

class UExplorationUIWidget;
class UCombatUIWidget;
class UTacticalUIWidget;
class UCombatHUDPresenter;
class UExplorationHUDPresenter;
class UMainMenuPresenter;
class UInventoryPresenter;

UCLASS()
class JRPG_API AJRPGHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, Category = "UI|CLasses")
	TSubclassOf<UMainMenuUIWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UExplorationUIWidget> ExplorationWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UCombatUIWidget> CombatWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UTacticalUIWidget> TacticalWidgetClass;

	UFUNCTION(BlueprintPure, Category = "UI|Presenter")
	UInventoryPresenter* GetInventoryPresenter() const { return InventoryPresenter; }

	UFUNCTION(BlueprintPure, Category = "UI|Presenter")
	UExplorationHUDPresenter* GetExplorationPresenter() const { return ExplorationPresenter; }

	UFUNCTION(BlueprintPure) class UCombatHUDPresenter* GetCombatPresenter() const { return CombatPresenter; }
	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<class UDamageTextWidget> DamageTextClass;

	// 플레이어 컨트롤러에서 호출할 메뉴 토글 함수
	void ToggleMainMenu();

	void TogglePartyInfo();

	void ShowSkillAnnouncer(const FString& SkillName);

private:
	// --- 프레젠터  ---
	UPROPERTY()
	TObjectPtr<UExplorationHUDPresenter> ExplorationPresenter;

	UPROPERTY()
	TObjectPtr<UCombatHUDPresenter> CombatPresenter;

	UPROPERTY()
	TObjectPtr<UMainMenuPresenter> MainMenuPresenter;

	UPROPERTY()
	TObjectPtr<UInventoryPresenter> InventoryPresenter;

	void OnMainMenuTabSelected(EMainMenuTab Tab);

	// [콘솔 명령어]
public:
	UFUNCTION(Exec, Category = "JRPG|Test")
	void TestRegionName(const FString& RegionName);
	UFUNCTION(Exec, Category = "JRPG|Test")
	void TestPartyChat(const FString& Message);
};