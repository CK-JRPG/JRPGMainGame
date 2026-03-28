#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatActionPaletteWidget.generated.h"

class UHPComponent;
class UAPComponent;
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

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_HPBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_HP;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_APBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_AP;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_SPBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SP;

private:
	TWeakObjectPtr<UHPComponent> CachedHPComp;
	TWeakObjectPtr<UAPComponent> CachedAPComp;
	TWeakObjectPtr<USPComponent> CachedSPComp;

	void OnPlayerHPChanged(float OldHP, float NewHP, FName Reason);
	void OnPlayerAPChanged(int32 OldAP, int32 NewAP, FName Reason);
	void OnPlayerSPChanged(int32 OldSP, int32 NewSP, FName Reason);
};