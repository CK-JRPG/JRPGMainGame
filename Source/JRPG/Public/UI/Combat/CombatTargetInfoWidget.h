#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatTargetInfoWidget.generated.h"

class UHPComponent;
class UGroggyComponent;

UCLASS()
class JRPG_API UCombatTargetInfoWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetTarget(AActor* TargetActor);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget)) class UTextBlock* Text_TargetName;
	UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_TargetHP;
	UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_GroggyGauge;

private:
	TWeakObjectPtr<UHPComponent> CachedHPComp;
	TWeakObjectPtr<UGroggyComponent> CachedGroggyComp;
	
	void OnTargetHPChanged(float OldHP, float NewHP, FName Reason);
	void OnTargetGroggyChanged(bool bGroggy);
};