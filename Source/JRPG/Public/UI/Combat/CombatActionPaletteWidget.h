#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatActionPaletteWidget.generated.h"

class USPComponent;
class UProgressBar;
class UTextBlock;

UCLASS()
class JRPG_API UCombatActionPaletteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindPlayerCharacter(AActor* PlayerActor);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_SPBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SP;

private:
	TWeakObjectPtr<USPComponent> CachedSPComp;

	void OnPlayerSPChanged(int32 OldSP, int32 NewSP, FName Reason);
};