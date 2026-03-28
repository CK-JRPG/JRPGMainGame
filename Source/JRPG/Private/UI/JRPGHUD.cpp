#include "UI/JRPGHUD.h"
#include "UI/Exploration/ExplorationUIWidget.h"
#include "UI/Combat/CombatUIWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Combat/Battle/BattleSessionSubsystem.h"

// 로그를 쉽게 찍기 위한 매크로 정의
#define HUD_LOG(Format, ...) UE_LOG(LogTemp, Warning, TEXT("[JRPGHUD_DEBUG] " Format), ##__VA_ARGS__)

void AJRPGHUD::BeginPlay()
{
    Super::BeginPlay();
    
    // 1. HUD가 생성되었는지, 어떤 컨트롤러가 소유하고 있는지 확인
    HUD_LOG("=== BeginPlay 호출됨! ===");
    if (APlayerController* PC = GetOwningPlayerController()) {
        HUD_LOG("소유중인 PlayerController: %s", *PC->GetName());
    } else {
        HUD_LOG("경고: 소유중인 PlayerController가 없습니다!");
    }

    // 2. 위젯 클래스가 블루프린트에서 잘 할당되었는지 확인
    if (!ExplorationWidgetClass) { 
        HUD_LOG("에러: ExplorationWidgetClass가 설정되지 않았습니다! (BP_JRPGHUD 확인 필요)"); 
    } else {
        ExplorationWidget = CreateWidget<UExplorationUIWidget>(GetWorld(), ExplorationWidgetClass);
        if (ExplorationWidget) {
            ExplorationWidget->AddToViewport();
            HUD_LOG("성공: 탐험 위젯 생성 및 뷰포트 추가 완료.");
        }
    }
    
    if (!CombatWidgetClass) { 
        HUD_LOG("에러: CombatWidgetClass가 설정되지 않았습니다! (BP_JRPGHUD 확인 필요)"); 
    } else {
        CombatWidget = CreateWidget<UCombatUIWidget>(GetWorld(), CombatWidgetClass);
        if (CombatWidget) {
            CombatWidget->AddToViewport();
            CombatWidget->SetVisibility(ESlateVisibility::Hidden);
            HUD_LOG("성공: 전투 위젯 생성 완료 (현재 Hidden 상태).");
        }
    }

    // 3. 서브시스템 바인딩 및 현재 전투 상태 체크
    if (UWorld* World = GetWorld())
    {
        if (UBattleSessionSubsystem* BattleSub = World->GetSubsystem<UBattleSessionSubsystem>())
        {
            HUD_LOG("BattleSessionSubsystem 찾음. 델리게이트 바인딩 완료.");
            BattleSub->OnBattleStarted.AddUObject(this, &AJRPGHUD::OnBattleStarted);
            BattleSub->OnBattleEnded.AddUObject(this, &AJRPGHUD::OnBattleEnded);

            // 중요: BeginPlay 시점에 이미 전투 중인지 강제 확인
            bool bIsActive = BattleSub->IsBattleActive();
            HUD_LOG("현재 IsBattleActive 상태: %s", bIsActive ? TEXT("True") : TEXT("False"));
            
            if (bIsActive) {
                HUD_LOG("전투가 이미 활성화되어 있어 SwitchToCombatUI를 즉시 호출합니다.");
                SwitchToCombatUI();
            } else {
                SwitchToExplorationUI();
            }
        }
        else { 
            HUD_LOG("에러: BattleSessionSubsystem을 찾을 수 없습니다!"); 
        }
    }
}

void AJRPGHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    HUD_LOG("=== EndPlay 호출됨! 파괴 원인: %d ===", (int32)EndPlayReason);
    
    if (UWorld* World = GetWorld())
    {
        if (UBattleSessionSubsystem* BattleSub = World->GetSubsystem<UBattleSessionSubsystem>())
        {
            BattleSub->OnBattleStarted.RemoveAll(this);
            BattleSub->OnBattleEnded.RemoveAll(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AJRPGHUD::SwitchToCombatUI()
{
    HUD_LOG("SwitchToCombatUI() 실행됨!");
    
    if (ExplorationWidget) ExplorationWidget->SetVisibility(ESlateVisibility::Hidden);
    
    if (CombatWidget) 
    {
        CombatWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
        if (PlayerPawn) {
            HUD_LOG("PlayerPawn 확인됨: %s. InitializeCombatState 호출합니다.", *PlayerPawn->GetName());
            CombatWidget->InitializeCombatState(PlayerPawn);
        } else {
            HUD_LOG("치명적 경고: PlayerPawn이 NULL입니다! 빙의(Possess)가 아직 안 끝났을 수 있습니다.");
        }
    } else {
        HUD_LOG("에러: CombatWidget이 유효하지 않아 켤 수 없습니다.");
    }
}

void AJRPGHUD::SwitchToExplorationUI()
{
    HUD_LOG("SwitchToExplorationUI() 실행됨!");
    if (CombatWidget) CombatWidget->SetVisibility(ESlateVisibility::Hidden);
    if (ExplorationWidget) ExplorationWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void AJRPGHUD::OnBattleStarted(const FBattleSessionSnapshot& Snapshot)
{
    HUD_LOG("델리게이트 이벤트 수신: OnBattleStarted!");
    SwitchToCombatUI();
}

void AJRPGHUD::OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason)
{
    HUD_LOG("델리게이트 이벤트 수신: OnBattleEnded!");
    SwitchToExplorationUI();
}