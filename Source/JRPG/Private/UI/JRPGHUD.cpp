#include "UI/JRPGHUD.h"
#include "UI/Exploration/ExplorationUIWidget.h"
#include "UI/Combat/TacticalUIWidget.h"
#include "UI/Presenters/CombatHUDPresenter.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"

void AJRPGHUD::BeginPlay()
{
    Super::BeginPlay();

    if (ExplorationWidgetClass)
    {
        ExplorationWidget = CreateWidget<UExplorationUIWidget>(GetWorld(), ExplorationWidgetClass);
        if (ExplorationWidget) ExplorationWidget->AddToViewport();
    }

    if (TacticalWidgetClass)
    {
        TacticalWidget = CreateWidget<UTacticalUIWidget>(GetWorld(), TacticalWidgetClass);
        if (TacticalWidget) { TacticalWidget->AddToViewport(10); TacticalWidget->SetVisibility(ESlateVisibility::Hidden); }
    }

    if (UTacticalModeSubsystem* TacticalSub = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        TacticalSub->OnTacticalModeEntered.AddUObject(this, &AJRPGHUD::OnTacticalModeEntered);
        TacticalSub->OnTacticalModeExited.AddUObject(this, &AJRPGHUD::OnTacticalModeExited);
    }

    // 전투 UI 로직은 모두 프레젠터에게 위임
    CombatPresenter = NewObject<UCombatHUDPresenter>(this);
    CombatPresenter->Initialize(GetWorld(), CombatWidgetClass);
}

void AJRPGHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UTacticalModeSubsystem* TacticalSub = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        TacticalSub->OnTacticalModeEntered.RemoveAll(this);
        TacticalSub->OnTacticalModeExited.RemoveAll(this);
    }

    if (ExplorationWidget) { ExplorationWidget->RemoveFromParent(); ExplorationWidget = nullptr; }
    if (TacticalWidget) { TacticalWidget->RemoveFromParent(); TacticalWidget = nullptr; }
    if (CombatPresenter) { CombatPresenter->Shutdown(); CombatPresenter = nullptr; }

    Super::EndPlay(EndPlayReason);
}

void AJRPGHUD::OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot) {
    if (TacticalWidget) TacticalWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (ExplorationWidget) ExplorationWidget->SetVisibility(ESlateVisibility::Hidden);
}

void AJRPGHUD::OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot) {
    if (TacticalWidget) TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
    if (ExplorationWidget) ExplorationWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}