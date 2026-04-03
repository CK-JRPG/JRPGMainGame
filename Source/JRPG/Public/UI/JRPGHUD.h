#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "JRPGHUD.generated.h"

class UExplorationUIWidget;
class UCombatUIWidget;
class UTacticalUIWidget;
class UCombatHUDPresenter;

UCLASS()
class JRPG_API AJRPGHUD : public AHUD
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<UExplorationUIWidget> ExplorationWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<UCombatUIWidget> CombatWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<UTacticalUIWidget> TacticalWidgetClass;

private:
    UPROPERTY() TObjectPtr<UExplorationUIWidget> ExplorationWidget;
    UPROPERTY() TObjectPtr<UTacticalUIWidget> TacticalWidget;
    UPROPERTY() TObjectPtr<UCombatHUDPresenter> CombatPresenter;

    void OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot);
    void OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot);
};