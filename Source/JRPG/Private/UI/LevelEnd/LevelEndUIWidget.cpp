#include "UI/LevelEnd/LevelEndUIWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

void ULevelEndUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyFadeOpacity(0.f);
	if (Button_Restart)
	{
		Button_Restart->OnClicked.RemoveAll(this);
		Button_Restart->OnClicked.AddDynamic(this, &ULevelEndUIWidget::RestartCurrentLevel);
	}
}

void ULevelEndUIWidget::NativeDestruct()
{
	if (Button_Restart)
	{
		Button_Restart->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void ULevelEndUIWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bFadeActive)
	{
		return;
	}

	FadeElapsed += InDeltaTime;
	const float Alpha = FadeDuration > 0.f ? FMath::Clamp(FadeElapsed / FadeDuration, 0.f, 1.f) : 1.f;
	ApplyFadeOpacity(Alpha);

	if (Alpha >= 1.f)
	{
		bFadeActive = false;
	}
}

void ULevelEndUIWidget::ShowEndScreen()
{
	FadeElapsed = 0.f;
	bFadeActive = true;
	ApplyFadeOpacity(1.f);
}

void ULevelEndUIWidget::RestartCurrentLevel()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			PC->SetPause(false);
		}

		const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(World, true));
		UGameplayStatics::OpenLevel(World, CurrentLevelName);
	}
}

void ULevelEndUIWidget::ApplyFadeOpacity(float Alpha)
{
	Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

	if (Image_Fade)
	{
		FLinearColor Tint = Image_Fade->GetColorAndOpacity();
		Tint.A = Alpha;
		Image_Fade->SetColorAndOpacity(Tint);
	}
}
