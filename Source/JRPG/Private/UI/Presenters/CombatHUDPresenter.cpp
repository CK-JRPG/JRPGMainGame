#include "UI/Presenters/CombatHUDPresenter.h"
#include "UI/Combat/CombatUIWidget.h"
#include "UI/Combat/CombatActionPaletteWidget.h"
#include "UI/Combat/CombatTargetInfoWidget.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "UI/Combat/EnemyHPBarWidget.h"
#include "UI/Combat/TacticalUIWidget.h"
#include "UI/ViewModels/CombatViewModels.h"
#include "UI/Combat/DamageTextWidget.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"
#include "Components/WidgetComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"

void UCombatHUDPresenter::Initialize(UWorld* InWorld, TSubclassOf<UCombatUIWidget> WidgetClass, TSubclassOf<UTacticalUIWidget> TacticalClass)
{
    if (!InWorld) return;

    if (WidgetClass)
    {
        CombatWidget = CreateWidget<UCombatUIWidget>(InWorld, WidgetClass);
        if (CombatWidget)
        {
            CombatWidget->AddToViewport(0);
            CombatWidget->SetVisibility(ESlateVisibility::Hidden);

            // 공통 뷰모델 생성 (액션 팔레트, 타겟)
            ActionPaletteVM = NewObject<UActionPaletteViewModel>(this);
            ActionPaletteVM->OnSPUIUpdated.AddUObject(this, &UCombatHUDPresenter::OnActionPaletteSPUpdated);

            TargetVM = NewObject<UEnemyViewModel>(this);
            TargetVM->OnTargetNameUpdated.AddUObject(this, &UCombatHUDPresenter::OnTargetNameUpdated);
            TargetVM->OnTargetHPUpdated.AddUObject(this, &UCombatHUDPresenter::OnTargetHPUpdated);
            TargetVM->OnTargetGroggyUpdated.AddUObject(this, &UCombatHUDPresenter::OnTargetGroggyUpdated);
        }
    }

    if (TacticalClass)
	{
		TacticalWidget = CreateWidget<UTacticalUIWidget>(InWorld, TacticalClass);
		if (TacticalWidget)
		{
			TacticalWidget->AddToViewport(10); 
			TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

    if (UTacticalModeSubsystem* TacticalSub = InWorld->GetSubsystem<UTacticalModeSubsystem>())
    {
        TacticalSub->OnTacticalModeEntered.AddUObject(this, &UCombatHUDPresenter::OnTacticalModeEntered);
        TacticalSub->OnTacticalModeExited.AddUObject(this, &UCombatHUDPresenter::OnTacticalModeExited);
    }

    if (UBattleSessionSubsystem* BattleSub = InWorld->GetSubsystem<UBattleSessionSubsystem>())
    {
        BattleSub->OnBattleStarted.AddUObject(this, &UCombatHUDPresenter::OnBattleStarted);
        BattleSub->OnBattleEnded.AddUObject(this, &UCombatHUDPresenter::OnBattleEnded);

        if (BattleSub->IsBattleActive()) OnBattleStarted(FBattleSessionSnapshot());
    }
}

void UCombatHUDPresenter::Shutdown()
{
    if (ActionPaletteVM) ActionPaletteVM->Unbind();
    if (TargetVM) TargetVM->Unbind();
    for (auto& VM : PartyVMs) { if (VM) VM->Unbind(); }
    for (auto& VM : EnemyHPBarVMs) { if (VM) VM->Unbind(); }

    if (UWorld* World = GetWorld())
    {
        if (UBattleSessionSubsystem* BattleSub = World->GetSubsystem<UBattleSessionSubsystem>())
        {
            BattleSub->OnBattleStarted.RemoveAll(this);
            BattleSub->OnBattleEnded.RemoveAll(this);
        }

        if (UTacticalModeSubsystem* TacticalSub = World->GetSubsystem<UTacticalModeSubsystem>())
        {
            TacticalSub->OnTacticalModeEntered.RemoveAll(this);
            TacticalSub->OnTacticalModeExited.RemoveAll(this);
        }
    }

    if (CombatWidget) { CombatWidget->RemoveFromParent(); CombatWidget = nullptr; }
    if (TacticalWidget) { TacticalWidget->RemoveFromParent(); TacticalWidget = nullptr; }
}

void UCombatHUDPresenter::ShowDamageText(AActor* Target, float Damage, bool bIsCritical)
{
    if (!CombatWidget || !DamageTextClass || !Target) return;
    UCanvasPanel* Canvas = CombatWidget->GetDamageCanvas();
    if (!Canvas) return;

    UDamageTextWidget* DmgWidget = CreateWidget<UDamageTextWidget>(GetWorld(), DamageTextClass);
    if (DmgWidget) {
        Canvas->AddChild(DmgWidget);
        DmgWidget->InitializeDamage(Target, Damage, bIsCritical);
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(DmgWidget->Slot)) {
            Slot->SetAlignment(FVector2D(0.5f, 0.5f));
            Slot->SetAutoSize(true);
        }
    }
}

void UCombatHUDPresenter::OnBattleStarted(const FBattleSessionSnapshot& Snapshot)
{
    if (!CombatWidget) return;
    CombatWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    // 1. 액션 팔레트 바인딩
    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
        if (ActionPaletteVM) ActionPaletteVM->BindToPlayer(PlayerPawn);
    }

    // 2. 파티 슬롯 바인딩
    if (CombatWidget->PartyRosterPanel)
    {
        CombatWidget->PartyRosterPanel->ClearRoster();
        for (auto& VM : PartyVMs) { if (VM) VM->Unbind(); }
        PartyVMs.Empty();

        if (UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>())
        {
            for (ACombatCharacterActor* Actor : SpawnSub->GetSpawnedActors())
            {
                if (!Actor || !CombatWidget->PartyRosterPanel->PartySlotClass) continue;
                UCombatPartySlotWidget* SlotWidget = CreateWidget<UCombatPartySlotWidget>(GetWorld(), CombatWidget->PartyRosterPanel->PartySlotClass);

                UCombatPartySlotViewModel* SlotVM = NewObject<UCombatPartySlotViewModel>(this);
                SlotVM->OnNameUpdated.AddUObject(this, &UCombatHUDPresenter::OnPartySlotNameUpdated, SlotWidget);
                SlotVM->OnHPUIUpdated.AddUObject(this, &UCombatHUDPresenter::OnPartySlotHPUpdated, SlotWidget);
                SlotVM->OnAPUIUpdated.AddUObject(this, &UCombatHUDPresenter::OnPartySlotAPUpdated, SlotWidget);

                SlotVM->BindToActor(Actor);

                PartyVMs.Add(SlotVM);

                CombatWidget->PartyRosterPanel->AddPartySlot(SlotWidget);
            }
        }
    }

    // 3. 적군 세팅 (제노블레이드식 메인 타겟팅 & 머리 위 HP바 연동)
    for (auto& VM : EnemyHPBarVMs) { if (VM) VM->Unbind(); }
    EnemyHPBarVMs.Empty();

    if (UBattleSessionSubsystem* BattleSub = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
    {
        TArray<AActor*> ActiveEnemies;
        BattleSub->GetAliveParticipantsByTeam(ECombatTeam::Enemy, ActiveEnemies);

        // 첫 번째 적을 메인 타겟 정보창에 임시 바인딩 (추후 타겟팅 시스템과 연동 필요)
        if (ActiveEnemies.Num() > 0 && TargetVM) {
            TargetVM->BindToEnemy(ActiveEnemies[0]);
        }

        for (AActor* Enemy : ActiveEnemies)
        {
            if (UWidgetComponent* HPBarComp = Enemy->FindComponentByClass<UWidgetComponent>())
            {
                HPBarComp->SetVisibility(true);
                if (UEnemyHPBarWidget* HPWidget = Cast<UEnemyHPBarWidget>(HPBarComp->GetUserWidgetObject()))
                {
                    UEnemyViewModel* EnemyVM = NewObject<UEnemyViewModel>(this);
                    EnemyVM->OnTargetHPUpdated.AddUObject(this, &UCombatHUDPresenter::OnEnemyHPBarUpdated, HPWidget);
                    EnemyVM->BindToEnemy(Enemy);
                    EnemyHPBarVMs.Add(EnemyVM);
                }
            }
        }
    }
}

void UCombatHUDPresenter::OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason)
{
    if (CombatWidget) CombatWidget->SetVisibility(ESlateVisibility::Hidden);
    if (ActionPaletteVM) ActionPaletteVM->Unbind();
    if (TargetVM) TargetVM->Unbind();
    for (auto& VM : PartyVMs) { if (VM) VM->Unbind(); }
    for (auto& VM : EnemyHPBarVMs) { if (VM) VM->Unbind(); }

    if (UBattleSessionSubsystem* BattleSub = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
    {
        TArray<AActor*> ActiveEnemies;
        BattleSub->GetAliveParticipantsByTeam(ECombatTeam::Enemy, ActiveEnemies);
        for (AActor* Enemy : ActiveEnemies)
        {
            if (UWidgetComponent* HPBarComp = Enemy->FindComponentByClass<UWidgetComponent>())
            {
                HPBarComp->SetVisibility(false);
            }
        }
    }
}

void UCombatHUDPresenter::OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot)
{
    if (TacticalWidget)
    {
        TacticalWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

void UCombatHUDPresenter::OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot)
{
    if (TacticalWidget)
    {
        TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

// 뷰모델 -> 뷰 토스 (중개 콜백 구현)
void UCombatHUDPresenter::OnActionPaletteSPUpdated(float Percent, const FString& Text) {
    if (CombatWidget && CombatWidget->ActionPalettePanel) CombatWidget->ActionPalettePanel->UpdateSPUI(Percent, Text);
}

void UCombatHUDPresenter::OnTargetNameUpdated(const FString& Name) {
    if (CombatWidget && CombatWidget->TargetInfoPanel) CombatWidget->TargetInfoPanel->UpdateTargetName(Name);
}

void UCombatHUDPresenter::OnTargetHPUpdated(float Percent, const FString& Text) {
    if (CombatWidget && CombatWidget->TargetInfoPanel) CombatWidget->TargetInfoPanel->UpdateTargetHP(Percent);
}

void UCombatHUDPresenter::OnTargetGroggyUpdated(bool bGroggy) {
    if (CombatWidget && CombatWidget->TargetInfoPanel) CombatWidget->TargetInfoPanel->UpdateGroggyState(bGroggy);
}

void UCombatHUDPresenter::OnPartySlotNameUpdated(const FString& Name, UCombatPartySlotWidget* View)
{
    if (View) View->UpdateName(Name);
}

void UCombatHUDPresenter::OnPartySlotHPUpdated(float Percent, const FString& Text, UCombatPartySlotWidget* View)
{
    if (View) View->UpdateHP(Percent, Text);
}

void UCombatHUDPresenter::OnPartySlotAPUpdated(float Percent, UCombatPartySlotWidget* View)
{
    if (View) View->UpdateAP(Percent);
}

void UCombatHUDPresenter::OnEnemyHPBarUpdated(float Percent, const FString& Text, UEnemyHPBarWidget* View)
{
    if (View) View->UpdateHP(Percent);
}