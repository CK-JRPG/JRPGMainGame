#include "UI/Combat/DamageTextWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/SlateColor.h"

void UDamageTextWidget::NativeConstruct() {
	Super::NativeConstruct();
	if (Anim_FloatAndFade) {
		FWidgetAnimationDynamicEvent EndDelegate;
		EndDelegate.BindDynamic(this, &UDamageTextWidget::OnFloatAnimFinished);
		BindToAnimationFinished(Anim_FloatAndFade, EndDelegate);
	}
}

void UDamageTextWidget::InitializeDamage(AActor* InTarget, float DamageAmount, bool bIsCritical, EDamageTextType TextType) 
{
	TargetActor = InTarget;
	WorldOffset = FVector(0.f, 0.f, 100.f);
	if (Text_Damage) 
	{
		int32 FinalValue = FMath::Abs(FMath::RoundToInt(DamageAmount));
		Text_Damage->SetText(FText::AsNumber(FinalValue));

		FSlateColor TextColor = FSlateColor(FLinearColor::White);

		switch (TextType)
		{
		case EDamageTextType::EnemyDamage:
			break;
		case EDamageTextType::PlayerDamage:
			TextColor = FSlateColor(FLinearColor::Red);
			break;
		case EDamageTextType::Heal:
			TextColor = FSlateColor(FLinearColor::Green);
			break;
		default:
			break;
		}

		Text_Damage->SetColorAndOpacity(TextColor);
	}

	if (Anim_FloatAndFade) PlayAnimation(Anim_FloatAndFade);
}

void UDamageTextWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime) {
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!TargetActor.IsValid()) return;
	FVector2D ScreenPos;
	if (GetOwningPlayer()->ProjectWorldLocationToScreen(TargetActor->GetActorLocation() + WorldOffset, ScreenPos, true)) {
		float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot)) CanvasSlot->SetPosition(ScreenPos / Scale);
	}
}

void UDamageTextWidget::OnFloatAnimFinished()
{
	SetVisibility(ESlateVisibility::Collapsed);

	OnDamageTextFinished.ExecuteIfBound(this);
}