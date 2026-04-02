#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatUIWidget.generated.h"

class UCombatTargetInfoWidget;
class UCombatPartyRosterWidget;
class UCombatActionPaletteWidget;

UCLASS()
class JRPG_API UCombatUIWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCombatTargetInfoWidget> TargetInfoPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCombatPartyRosterWidget> PartyRosterPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCombatActionPaletteWidget> ActionPalettePanel;
};