#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyIndicatorWidget.generated.h"

UENUM(BlueprintType)
enum class EEnemyAggroState : uint8
{
	Nearby = 0,
	Chasing = 1
};

UCLASS()
class JRPG_API UEnemyIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateIndicator(EEnemyAggroState State, float AngleDegree);

protected:
	UPROPERTY(meta = (BindWidget))
	class UWidget* Box_Rotator;

	UPROPERTY(meta = (BindWidget))
	class UImage* Image_Arrow;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Colors")
	FLinearColor Color_Nearby = FLinearColor(1.f, 1.f, 0.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "UI|Colors")
	FLinearColor Color_Chasing = FLinearColor(1.f, 0.f, 0.f, 1.f);
};
