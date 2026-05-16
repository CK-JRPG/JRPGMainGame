#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Tactical/TacticalModeTypes.h"
#include "UI/Combat/DamageTextWidget.h"
#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
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

    void ShowDamageText(AActor* Target, float Damage, bool bIsCritical, EDamageTextType TextType);
    UPROPERTY() TSubclassOf<class UDamageTextWidget> DamageTextClass;
    void OnActiveCharacterChanged(FName NewActiveID);
    void ShowSkillAnnouncer(const FString& SkillName);
    void BeginEncounterIntro();
    void EndEncounterIntro();
    void UpdateTargetEnemyUI(AActor* NewTarget);

    void SetPartyWheelActive(bool bActive);
    FName GetHoveredPartyMemberID() const;

    void UpdateTargetInfo();

private:
    bool isPlayEncounter;

    UPROPERTY() TObjectPtr<UCombatUIWidget> CombatWidget;
    UPROPERTY() TObjectPtr<UTacticalUIWidget> TacticalWidget;
    UPROPERTY() TObjectPtr<class UActionPaletteViewModel> ActionPaletteVM;

    UPROPERTY() TObjectPtr<class UEnemyViewModel> TargetVM;
    UPROPERTY() TArray<TObjectPtr<class UCombatPartySlotViewModel>> PartyVMs;
    UPROPERTY() TArray<TObjectPtr<class UEnemyViewModel>> EnemyHPBarVMs;
    UPROPERTY() TArray<TObjectPtr<class UDamageTextWidget>> DamageTextPool;
    UPROPERTY() TArray<TObjectPtr<class UCombatPartySlotViewModel>> TagSwapVMs;
    UPROPERTY() TArray<TWeakObjectPtr<class UHPComponent>> BoundHPComps;

    void ReturnDamageTextToPool(class UDamageTextWidget* Widget);

    void OnBattleStarted(const FBattleSessionSnapshot& Snapshot);
    void OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);

    void OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot);
    void OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot);

    void OnActionPaletteSPUpdated(float Percent, const FString& Text);
    //void OnTargetNameUpdated(const FString& Name);
    void OnTargetHPUpdated(float Percent, const FString& Text);
    //void OnTargetGroggyUpdated(bool bGroggy);

    void OnPartySlotNameUpdated(const FString& Name, class UCombatPartySlotWidget* View);
    void OnPartySlotHPUpdated(float Percent, const FString& Text, class UCombatPartySlotWidget* View);
    void OnPartySlotAPUpdated(float Percent, class UCombatPartySlotWidget* View);

    void OnEnemyHPBarUpdated(float Percent, const FString& Text, class UEnemyHPBarWidget* View);

    void HandleActorHPChangedForDamageText(float OldHP, float NewHP, FName Reason, AActor* TargetActor);

    class UCombatPartySlotViewModel* GetPartySLotVM(FName CharID);

    void ClearHPBindings();

    // Action Palette용
    UPROPERTY()
    TMap<FName, TObjectPtr<class UCombatPartySlotWidget>> PartySlotWidgets;

    TWeakObjectPtr<class UCombatPartySlotViewModel> CurrentActivePartyVM;

    TWeakObjectPtr<AActor> LastTargetActor;
    AActor* FindSoftTargetEnemy() const;

    void OnActionPaletteHPUpdated(float Percent, const FString& Text);
    void OnActionPaletteAPUpdated(float Percent);
    void OnActionPaletteSkillUpdated(const TArray<FString>& SkillNames);

    void OnCombatPresentationStarted(EPresentedCombatActionType ActionType, FName ActionId);

    void HandleSkillCooldownFinished(FName SkillId, FName CharacterID);
};
