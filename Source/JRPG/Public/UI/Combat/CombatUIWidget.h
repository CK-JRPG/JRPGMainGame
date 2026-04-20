#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatUIWidget.generated.h"

class UCombatTargetInfoWidget;
class UCombatPartyRosterWidget;
class UCombatActionPaletteWidget;
class UCanvasPanel;
class UCombatTagSwapWidget;

UCLASS()
class JRPG_API UCombatUIWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatTargetInfoWidget> TargetInfoPanel;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatPartyRosterWidget> PartyRosterPanel;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatTagSwapWidget> TagSwapPanel;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UCombatActionPaletteWidget> ActionPalettePanel;
    UCanvasPanel* GetDamageCanvas() const { return Canvas_Damage; }

    void PlaySkillAnnouncer(const FString& SkillName);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCanvasPanel> Canvas_Damage;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_SkillAnnouncer;
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    class UWidgetAnimation* Anim_SkillAnnouncer;
};