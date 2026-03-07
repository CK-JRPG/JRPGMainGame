#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CombatDebugHUD.generated.h"

UCLASS()
class JRPG_API ACombatDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere) float LeftX = 40.f;
	UPROPERTY(EditAnywhere) float TopY = 40.f;
	UPROPERTY(EditAnywhere) float LineHeight = 16.f;

	UPROPERTY(EditAnywhere) bool bShowSummary = true;
};