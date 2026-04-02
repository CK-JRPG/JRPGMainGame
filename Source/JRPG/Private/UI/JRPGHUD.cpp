#include "UI/JRPGHUD.h"
#include "UI/Exploration/ExplorationUIWidget.h"
#include "UI/Combat/CombatUIWidget.h"
#include "UI/Combat/TacticalUIWidget.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Components/WidgetComponent.h"

// =========================================================
// [디버그 로그 스위치]
// 1로 설정하면 로그가 출력되고, 0으로 설정하면 모든 HUD_LOG가 무시됩니다.
#define ENABLE_HUD_DEBUG_LOG 0
// =========================================================

#if ENABLE_HUD_DEBUG_LOG
    #define HUD_LOG(Format, ...) UE_LOG(LogTemp, Warning, TEXT("[JRPGHUD_DEBUG] " Format), ##__VA_ARGS__)
#else
    #define HUD_LOG(Format, ...)
#endif

void AJRPGHUD::BeginPlay()
{
    Super::BeginPlay();
    
    if (CombatWidgetClass)
    {
        CombatWidget = CreateWidget<UCombatUIWidget>(GetWorld(), CombatWidgetClass);
        if (CombatWidget)
        {
            CombatWidget->AddToViewport(0); // 기본 Z-Order
            CombatWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    
    if (TacticalWidgetClass)
    {
        TacticalWidget = CreateWidget<UTacticalUIWidget>(GetWorld(), TacticalWidgetClass);
        if (TacticalWidget)
        {
            // CombatUI(0)보다 무조건 위에 그려지도록 Z-Order를 10으로 설정합니다.
            TacticalWidget->AddToViewport(10); 
            TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    
    if (UTacticalModeSubsystem* TacticalSub = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        TacticalSub->OnTacticalModeEntered.AddUObject(this, &AJRPGHUD::OnTacticalModeEntered);
        TacticalSub->OnTacticalModeExited.AddUObject(this, &AJRPGHUD::OnTacticalModeExited);
    }
    
    HUD_LOG("=== BeginPlay 호출됨! ===");
    if (APlayerController* PC = GetOwningPlayerController()) {
        HUD_LOG("소유중인 PlayerController: %s", *PC->GetName());
    } else {
        HUD_LOG("경고: 소유중인 PlayerController가 없습니다!");
    }
    
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
    HUD_LOG("=== EndPlay 호출됨! 위젯 청소 및 파괴 원인: %d ===", (int32)EndPlayReason);
    
    // 델리게이트 바인딩 해제 (메모리 누수 방지)
    if (UWorld* World = GetWorld())
    {
        if (UBattleSessionSubsystem* BattleSub = World->GetSubsystem<UBattleSessionSubsystem>())
        {
            BattleSub->OnBattleStarted.RemoveAll(this);
            BattleSub->OnBattleEnded.RemoveAll(this);
        }
    }

    // 뷰포트에 남아있는 고아 위젯들을 강제로 제거
    if (ExplorationWidget)
    {
        ExplorationWidget->RemoveFromParent();
        ExplorationWidget = nullptr;
    }

    if (CombatWidget)
    {
        CombatWidget->RemoveFromParent();
        CombatWidget = nullptr;
    }
    
    if (TacticalWidget)
    {
        TacticalWidget->RemoveFromParent();
        TacticalWidget = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AJRPGHUD::OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot)
{
    if (TacticalWidget)
    {
        TacticalWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

void AJRPGHUD::OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot)
{
    if (TacticalWidget)
    {
        TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
    }
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
            //CombatWidget->InitializeCombatState(PlayerPawn);
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
    if (UBattleSessionSubsystem* BattleSubsystem = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
    {
        TArray<AActor*> ActiveEnemies;
        // 현재 배틀 세션에 살아있는 적 팀 목록을 가져옵니다.
        BattleSubsystem->GetAliveParticipantsByTeam(ECombatTeam::Enemy, ActiveEnemies);

        for (AActor* EnemyActor : ActiveEnemies)
        {
            if (EnemyActor)
            {
                // 액터가 가진 위젯 컴포넌트를 찾아 Visibility를 켭니다.
                if (UWidgetComponent* HPBarComp = EnemyActor->FindComponentByClass<UWidgetComponent>())
                {
                    HPBarComp->SetVisibility(true);
                }
            }
        }
    }
}

void AJRPGHUD::OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason)
{
    HUD_LOG("델리게이트 이벤트 수신: OnBattleEnded!");
    SwitchToExplorationUI();
    if (UBattleSessionSubsystem* BattleSubsystem = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
    {
        TArray<AActor*> ActiveEnemies;
        BattleSubsystem->GetAliveParticipantsByTeam(ECombatTeam::Enemy, ActiveEnemies); //

        for (AActor* EnemyActor : ActiveEnemies)
        {
            if (EnemyActor)
            {
                if (UWidgetComponent* HPBarComp = EnemyActor->FindComponentByClass<UWidgetComponent>())
                {
                    HPBarComp->SetVisibility(false);
                }
            }
        }
    }
}