#include "UI/JRPGHUD.h"
#include "UI/Presenters/ExplorationHUDPresenter.h"
#include "UI/Presenters/CombatHUDPresenter.h"
#include "UI/Presenters/MainMenuPresenter.h"
#include "UI/Presenters/InventoryPresenter.h"
#include "UI/Widgets/InventoryUIWidget.h"

void AJRPGHUD::BeginPlay()
{
    Super::BeginPlay();

    // 메인 메뉴 프레젠터 초기화
    if (MainMenuWidgetClass)
    {
        MainMenuPresenter = NewObject<UMainMenuPresenter>(this);
        MainMenuPresenter->Initialize(GetWorld(), MainMenuWidgetClass);
        MainMenuPresenter->OnTabSelected.AddUObject(this, &AJRPGHUD::OnMainMenuTabSelected);

        // 2. 인벤토리 프레젠터 초기화 (메인 메뉴 안의 위젯을 넘겨줌)
        if (UMainMenuUIWidget* MenuUI = MainMenuPresenter->GetWidget())
        {
            InventoryPresenter = NewObject<UInventoryPresenter>(this);
            InventoryPresenter->InitializeWithExistingWidget(GetWorld(), MenuUI->GetInventoryWidget());
        }
    }

    // 탐험 UI 프레젠터 초기화 (미니맵, 퀘스트)
    ExplorationPresenter = NewObject<UExplorationHUDPresenter>(this);
    ExplorationPresenter->Initialize(GetWorld(), ExplorationWidgetClass);

    // 전투 UI 프레젠터 초기화 (파티, 스탯)
    CombatPresenter = NewObject<UCombatHUDPresenter>(this);
    CombatPresenter->Initialize(GetWorld(), CombatWidgetClass, TacticalWidgetClass);
}

void AJRPGHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 프레젠터 및 위젯 메모리 정리 (고아 위젯 방지)
    if (MainMenuPresenter) { MainMenuPresenter->Shutdown(); MainMenuPresenter = nullptr; }
    if (ExplorationPresenter) { ExplorationPresenter->Shutdown(); ExplorationPresenter = nullptr; }
    if (CombatPresenter) { CombatPresenter->Shutdown(); CombatPresenter = nullptr; }

    Super::EndPlay(EndPlayReason);
}

void AJRPGHUD::ToggleMainMenu()
{
    if (MainMenuPresenter)
    {
		//UE_LOG(LogTemp, Warning, TEXT("JRPGHUD::ToggleMainMenu: 메뉴 토글"));
        MainMenuPresenter->ToggleMenu();
    }
}

void AJRPGHUD::TogglePartyInfo()
{
    // 프레젠터가 살아있다면 탭 변경을 지시합니다.
    if (ExplorationPresenter)
    {
        ExplorationPresenter->TogglePartyInfo();
    }
    else UE_LOG(LogTemp, Error, TEXT("JRPGHUD::TogglePartyInfo: ExplorationPresenter Invaild"));
}

void AJRPGHUD::OnMainMenuTabSelected(EMainMenuTab Tab)
{
    // 탭이 인벤토리(2번)로 바뀔 때만 데이터를 갱신합니다.
    if (Tab == EMainMenuTab::Inventory && InventoryPresenter)
    {
        InventoryPresenter->OpenInventory(GetOwningPawn());
    }
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
