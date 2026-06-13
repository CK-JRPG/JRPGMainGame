#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "CombatUIWidget.generated.h"

class UCombatTargetInfoWidget;
class UCombatPartyRosterWidget;
class UCombatActionPaletteWidget;
class UCanvasPanel;
class UCombatTagSwapWidget;
class UCombatEncounterOverlayWidget;

UCLASS()
class JRPG_API UCombatUIWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatTargetInfoWidget> TargetInfoPanel;
    //UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatPartyRosterWidget> PartyRosterPanel;
    //UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatTagSwapWidget> TagSwapPanel;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatActionPaletteWidget> ActionPalettePanel;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatEncounterOverlayWidget> EncounterOverlayPanel;

    UPROPERTY(meta = (BindWidget)) UOverlay* LogOverlayWidget;

    UCanvasPanel* GetDamageCanvas() const { return Canvas_Damage; }

    void PlaySkillAnnouncer(const FString& SkillName);
    void SetCombatPanelsVisible(bool bVisible);
    void ShowEncounterOverlay();
    void HideEncounterOverlay();
    void AddCombatLog(const FString& Message, class UTexture2D* Icon);

    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<class UCombatLogEntry> LogEntryClass;

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCanvasPanel> Canvas_Damage;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_SkillAnnouncer;

    UPROPERTY(Transient, meta = (BindWidgetAnim))
    class UWidgetAnimation* Anim_SkillAnnouncer;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    class UWidgetAnimation* Anim_EncounterIntro;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<class UVerticalBox> VB_CombatLog;
   };
