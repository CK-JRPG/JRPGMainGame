#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "CombatHUDPresenter.generated.h"

UCLASS()
class JRPG_API UCombatHUDPresenter : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UWorld* InWorld, TSubclassOf<class UCombatUIWidget> WidgetClass);
    void Shutdown();

private:
    UPROPERTY() TObjectPtr<class UCombatUIWidget> CombatWidget;
    UPROPERTY() TObjectPtr<class UActionPaletteViewModel> ActionPaletteVM;
    UPROPERTY() TObjectPtr<class UEnemyViewModel> TargetVM;
    UPROPERTY() TArray<TObjectPtr<class UCombatPartySlotViewModel>> PartyVMs;
    UPROPERTY() TArray<TObjectPtr<class UEnemyViewModel>> EnemyHPBarVMs;

    void OnBattleStarted(const FBattleSessionSnapshot& Snapshot);
    void OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);

    // 중개 콜백들 (VM -> View)
    void OnActionPaletteSPUpdated(float Percent, const FString& Text);
    void OnTargetNameUpdated(const FString& Name);
    void OnTargetHPUpdated(float Percent, const FString& Text);
    void OnTargetGroggyUpdated(bool bGroggy);

    void OnPartySlotNameUpdated(const FString& Name, class UCombatPartySlotWidget* View);
    void OnPartySlotHPUpdated(float Percent, const FString& Text, class UCombatPartySlotWidget* View);
    void OnPartySlotAPUpdated(float Percent, class UCombatPartySlotWidget* View);

    void OnEnemyHPBarUpdated(float Percent, const FString& Text, class UEnemyHPBarWidget* View);
};