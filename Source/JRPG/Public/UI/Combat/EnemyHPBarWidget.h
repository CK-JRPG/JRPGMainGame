#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHPBarWidget.generated.h"

class UProgressBar;
class UHPComponent;

UCLASS()
class JRPG_API UEnemyHPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindHPComponent(UHPComponent* InHPComp);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HPBar;

private:
	TWeakObjectPtr<UHPComponent> CachedHPComp;

	void OnHPChanged(float OldHP, float NewHP, FName Reason);
};