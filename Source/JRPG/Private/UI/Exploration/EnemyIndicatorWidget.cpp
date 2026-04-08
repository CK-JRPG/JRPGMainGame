#include "UI/Exploration/EnemyIndicatorWidget.h"

#include "Components/Image.h"
#include "Components/Widget.h"

void UEnemyIndicatorWidget::UpdateIndicator(EEnemyAggroState State, float AngleDegree)
{
	if (Image_Arrow)
	{
		Image_Arrow->SetColorAndOpacity(State == EEnemyAggroState::Chasing ? Color_Chasing : Color_Nearby);
	}

	if (Box_Rotator)
	{
		Box_Rotator->SetRenderTransformAngle(AngleDegree);
	}
}
