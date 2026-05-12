#include "UI/JRPGHUD.h"
#include "UI/Presenters/ExplorationHUDPresenter.h"
#include "UI/Presenters/CombatHUDPresenter.h"
#include "UI/Presenters/MainMenuPresenter.h"
#include "UI/Presenters/InventoryPresenter.h"
#include "UI/Widgets/InventoryUIWidget.h"
#include "Combat/Battle/BattleSessionSubsystem.h"

void AJRPGHUD::BeginPlay()
{
    Super::BeginPlay();

    // 메인 메뉴 프레젠터 초기화
    if (MainMenuWidgetClass)
    {
        MainMenuPresenter = NewObject<UMainMenuPresenter>(this);
        MainMenuPresenter->Initialize(GetWorld(), MainMenuWidgetClass);
        MainMenuPresenter->OnTabSelected.AddUObject(this, &AJRPGHUD::OnMainMenuTabSelected);

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
    if (CombatPresenter) CombatPresenter->DamageTextClass = DamageTextClass;
}

void AJRPGHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (MainMenuPresenter) { MainMenuPresenter->Shutdown(); MainMenuPresenter = nullptr; }
    if (ExplorationPresenter) { ExplorationPresenter->Shutdown(); ExplorationPresenter = nullptr; }
    if (CombatPresenter) { CombatPresenter->Shutdown(); CombatPresenter = nullptr; }

    Super::EndPlay(EndPlayReason);
}

void AJRPGHUD::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (CombatPresenter)
    {
        if (UBattleSessionSubsystem* BattleSub = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
        {
            if (BattleSub->IsBattleActive())
            {
                CombatPresenter->UpdateTargetInfo();
            }
        }
    }
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
    if (ExplorationPresenter)
    {
        ExplorationPresenter->TogglePartyInfo();
    }
    else UE_LOG(LogTemp, Error, TEXT("JRPGHUD::TogglePartyInfo: ExplorationPresenter Invaild"));
}

void AJRPGHUD::ShowSkillAnnouncer(const FString& SkillName)
{
    if (CombatPresenter)
    {
        CombatPresenter->ShowSkillAnnouncer(SkillName);
    }
}

void AJRPGHUD::OnMainMenuTabSelected(EMainMenuTab Tab)
{
    // 탭이 인벤토리(2번)로 바뀔 때만 데이터를 갱신
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
