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
    
	UPROPERTY(meta = (BindWidget)) class UTextBlock* Text_Name;
	UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_MemberHP;

private:
	TWeakObjectPtr<class UHPComponent> CachedHPComp;
	void OnHPChanged(float OldHP, float NewHP, FName Reason);
};