#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "CombatHUDPresenter.generated.h"

class UCombatUIWidget;
class UTacticalUIWidget;

UCLASS()
class JRPG_API UCombatHUDPresenter : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UWorld* InWorld, TSubclassOf<UCombatUIWidget> WidgetClass, TSubclassOf<UTacticalUIWidget> TacticalClass);
    void Shutdown();

    void ShowDamageText(AActor* Target, float Damage, bool bIsCritical);
    UPROPERTY() TSubclassOf<class UDamageTextWidget> DamageTextClass;

private:
    UPROPERTY() TObjectPtr<UCombatUIWidget> CombatWidget;
    UPROPERTY() TObjectPtr<UTacticalUIWidget> TacticalWidget;
    UPROPERTY() TObjectPtr<class UActionPaletteViewModel> ActionPaletteVM;
    UPROPERTY() TObjectPtr<class UEnemyViewModel> TargetVM;
    UPROPERTY() TArray<TObjectPtr<class UCombatPartySlotViewModel>> PartyVMs;
    UPROPERTY() TArray<TObjectPtr<class UEnemyViewModel>> EnemyHPBarVMs;

    UPROPERTY()
    TArray<TObjectPtr<class UDamageTextWidget>> DamageTextPool;

    void ReturnDamageTextToPool(class UDamageTextWidget* Widget);

    void OnBattleStarted(const FBattleSessionSnapshot& Snapshot);
    void OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);

    void OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot);
    void OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot);

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