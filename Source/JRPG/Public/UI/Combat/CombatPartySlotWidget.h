#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatPartySlotWidget.generated.h"

UCLASS()
class JRPG_API UCombatPartySlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void BindPartyMember(AActor* MemberActor);
protected:
	virtual void NativeDestruct() override;
    
	UPROPERTY(meta = (BindWidget)) class UImage* Image_Portrait;
	UPROPERTY(meta = (BindWidget)) class UTextBlock* Text_Name;
	UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_HPBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_HP;
	UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_APBar;

private:
	TWeakObjectPtr<class UHPComponent> CachedHPComp;
	TWeakObjectPtr<class UAPComponent> CachedAPComp;

	void OnHPChanged(float OldHP, float NewHP, FName Reason);
	void OnAPChanged(int32 OldAP, int32 NewAP, FName Reason);
};