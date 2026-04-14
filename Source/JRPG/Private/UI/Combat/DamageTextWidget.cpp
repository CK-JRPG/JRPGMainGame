#include "UI/Combat/DamageTextWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UDamageTextWidget::NativeConstruct() {
	Super::NativeConstruct();
	if (Anim_FloatAndFade) {
		FWidgetAnimationDynamicEvent EndDelegate;
		EndDelegate.BindDynamic(this, &UDamageTextWidget::OnFloatAnimFinished);
		BindToAnimationFinished(Anim_FloatAndFade, EndDelegate);
	}
}

void UDamageTextWidget::InitializeDamage(AActor* InTarget, float DamageAmount, bool bIsCritical) {
	TargetActor = InTarget;
	WorldOffset = FVector(0.f, 0.f, 100.f);
	if (Text_Damage) {
		FString DmgStr = FString::FromInt(FMath::RoundToInt(DamageAmount));
		Text_Damage->SetText(FText::FromString(bIsCritical ? TEXT("!") + DmgStr + TEXT("!") : DmgStr));
		Text_Damage->SetColorAndOpacity(bIsCritical ? FLinearColor::Red : FLinearColor::White);
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

void UDamageTextWidget::OnFloatAnimFinished() { RemoveFromParent(); }