#include "UI/JRPGHUD.h"
#include "UI/Presenters/ExplorationHUDPresenter.h"
#include "UI/Presenters/CombatHUDPresenter.h"
#include "UI/Presenters/MainMenuPresenter.h"
#include "UI/Presenters/InventoryPresenter.h"
#include "UI/Combat/TacticalUIWidget.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"

void AJRPGHUD::BeginPlay()
{
    Super::BeginPlay();

    // 메인 메뉴 프레젠터 초기화
    if (MainMenuWidgetClass)
    {
        MainMenuPresenter = NewObject<UMainMenuPresenter>(this);
    }

    // 탐험 UI 프레젠터 초기화 (미니맵, 퀘스트)
    ExplorationPresenter = NewObject<UExplorationHUDPresenter>(this);
    ExplorationPresenter->Initialize(GetWorld(), ExplorationWidgetClass);

    // 전투 UI 프레젠터 초기화 (파티, 스탯)
    CombatPresenter = NewObject<UCombatHUDPresenter>(this);
    CombatPresenter->Initialize(GetWorld(), CombatWidgetClass);

    // 택티컬 UI 위젯 초기화
    if (TacticalWidgetClass)
    {
        TacticalWidget = CreateWidget<UTacticalUIWidget>(GetWorld(), TacticalWidgetClass);
        if (TacticalWidget)
        {
            TacticalWidget->AddToViewport(10);
            TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    // 택티컬 모드 이벤트 바인딩
    if (UTacticalModeSubsystem* TacticalSub = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        TacticalSub->OnTacticalModeEntered.AddUObject(this, &AJRPGHUD::OnTacticalModeEntered);
        TacticalSub->OnTacticalModeExited.AddUObject(this, &AJRPGHUD::OnTacticalModeExited);
    }
}

void AJRPGHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UTacticalModeSubsystem* TacticalSub = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        TacticalSub->OnTacticalModeEntered.RemoveAll(this);
        TacticalSub->OnTacticalModeExited.RemoveAll(this);
    }

    // 프레젠터 및 위젯 메모리 정리 (고아 위젯 방지)
    if (ExplorationPresenter) { ExplorationPresenter->Shutdown(); ExplorationPresenter = nullptr; }
    if (CombatPresenter) { CombatPresenter->Shutdown(); CombatPresenter = nullptr; }
    if (TacticalWidget) { TacticalWidget->RemoveFromParent(); TacticalWidget = nullptr; }

    Super::EndPlay(EndPlayReason);
}

void AJRPGHUD::ToggleMainMenu()
{
}

void AJRPGHUD::OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot)
{
    if (TacticalWidget) TacticalWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (ExplorationPresenter) ExplorationPresenter->HideExplorationUI();
}

void AJRPGHUD::OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot)
{
    if (TacticalWidget) TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
    if (ExplorationPresenter) ExplorationPresenter->ShowExplorationUI();
}

void AJRPGHUD::OnMainMenuTabSelected(EMainMenuTab Tab)
{
}

void AJRPGHUD::TestRegionName(const FString& RegionName)
{
    if (ExplorationPresenter)
    {
        ExplorationPresenter->TestRegionName(RegionName);
    }
}

void AJRPGHUD::TestPartyChat(const FString& Message)
{
    if (ExplorationPresenter)
    {
        ExplorationPresenter->TestPartyChat(Message);
    }
}
