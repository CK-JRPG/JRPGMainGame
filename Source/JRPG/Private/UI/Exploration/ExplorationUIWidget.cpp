#include "UI/Exploration/ExplorationUIWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/VerticalBox.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "UI/ViewModels/CombatViewModels.h"
#include "UI/Exploration/PartyChatBubbleWidget.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UExplorationUIWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!Canvas_Indicators || !IndicatorClass) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->PlayerCameraManager) return;

	FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
	FRotator CameraRot = PC->PlayerCameraManager->GetCameraRotation();

	// 뷰포트 크기가 아닌, DPI 스케일이 적용된 현재 UI(캔버스)의 실제 크기
	FVector2D CanvasSize = MyGeometry.GetLocalSize();
	FVector2D ScreenCenter = CanvasSize * 0.5f;

	float PaddingX = 40.0f;
	float PaddingY = 40.0f;
	float MaxX = ScreenCenter.X - PaddingX;
	float MaxY = ScreenCenter.Y - PaddingY;

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACombatCharacterActor::StaticClass(), FoundEnemies);

	int32 ActiveIndicatorCount = 0;

	for (AActor* Enemy : FoundEnemies)
	{
		if (!Enemy) continue;

		float Distance = FVector::Dist(CameraLoc, Enemy->GetActorLocation());
		if (Distance > DetectionRadius) continue;

		FVector EnemyLoc = Enemy->GetActorLocation();
		FVector DirToEnemy = EnemyLoc - CameraLoc;
		FVector LocalDir = CameraRot.UnrotateVector(DirToEnemy);

		FVector2D ScreenPosition;
		bool bIsOnScreen = PC->ProjectWorldLocationToScreen(EnemyLoc, ScreenPosition, true);

		float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
		FVector2D UMGPosition = ScreenPosition / DPIScale;

		if (bIsOnScreen && LocalDir.X > 0.0f)
		{
			if (UMGPosition.X >= 0 && UMGPosition.X <= CanvasSize.X &&
				UMGPosition.Y >= 0 && UMGPosition.Y <= CanvasSize.Y)
			{
				continue;
			}
		}

		FVector2D DirFromCenter = UMGPosition - ScreenCenter;

		if (LocalDir.X < 0.0f)
		{
			DirFromCenter *= -1.0f;
		}
		DirFromCenter.Normalize();

		// 테두리 교차점 계산
		float tX = (FMath::Abs(DirFromCenter.X) > KINDA_SMALL_NUMBER) ? (MaxX / FMath::Abs(DirFromCenter.X)) : 99999.0f;
		float tY = (FMath::Abs(DirFromCenter.Y) > KINDA_SMALL_NUMBER) ? (MaxY / FMath::Abs(DirFromCenter.Y)) : 99999.0f;
		float t = FMath::Min(tX, tY);

		FVector2D IndicatorPos = ScreenCenter + (DirFromCenter * t);
		float AngleDegree = FMath::RadiansToDegrees(FMath::Atan2(DirFromCenter.Y, DirFromCenter.X));

		EEnemyAggroState State = (Distance < 1000.0f) ? EEnemyAggroState::Chasing : EEnemyAggroState::Nearby;

		UEnemyIndicatorWidget* IndicatorObj = nullptr;
		if (ActiveIndicatorCount < CachedIndicatorWidgets.Num())
		{
			IndicatorObj = CachedIndicatorWidgets[ActiveIndicatorCount];
		}
		else
		{
			IndicatorObj = CreateWidget<UEnemyIndicatorWidget>(GetWorld(), IndicatorClass);
			CachedIndicatorWidgets.Add(IndicatorObj);
			Canvas_Indicators->AddChild(IndicatorObj);
		}

		IndicatorObj->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		IndicatorObj->UpdateIndicator(State, AngleDegree);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IndicatorObj->Slot))
		{
			CanvasSlot->SetPosition(IndicatorPos);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
		}

		ActiveIndicatorCount++;
	}

	for (int32 i = ActiveIndicatorCount; i < CachedIndicatorWidgets.Num(); ++i)
	{
		CachedIndicatorWidgets[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UExplorationUIWidget::UpdateQuestInfo(UTexture2D* QuestIcon, const FString& ObjectiveText)
{
	if (Text_QuestObjective)
	{
		Text_QuestObjective->SetText(FText::FromString(ObjectiveText));
	}

	if (Image_QuestIcon)
	{
		if (QuestIcon)
		{
			Image_QuestIcon->SetBrushFromTexture(QuestIcon);
			Image_QuestIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			// Image_QuestIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UExplorationUIWidget::SetPartyStatusMode(int32 Mode)
{
	if (!Widget_PartyStatus) return;

	if (Mode == 0) // 숨김
	{
		PlayPartyStatusAnim(false);
		//Widget_PartyStatus->SetVisibility(ESlateVisibility::Hidden);
		// (필요 시 지역명도 숨김 처리)
	}
	else if (Mode == 1) // 전투 후 회복 모드 (Tab 텍스트 등은 가리고 체력바만 보여주는 연출)
	{
		UE_LOG(LogTemp, Warning, TEXT("UExplorationUIWidget::SetPartyStatusMode : Mode = 회복 모드"));
		Widget_PartyStatus->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PlayPartyStatusAnim(true);
	}
	else if (Mode == 2) // Tab 전체 정보 모드
	{
		Widget_PartyStatus->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PlayPartyStatusAnim(true);
		PlayRegionNameAnimation();
	}
}

void UExplorationUIWidget::ShowInteraction(bool bShow, const FString& Text)
{
	if (Overlay_Interaction && Text_InteractionAction)
	{
		Overlay_Interaction->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Text_InteractionAction->SetText(FText::FromString(Text));
	}
}

void UExplorationUIWidget::ShowDialogue(bool bShow, const FString& Speaker, const FString& Text)
{
	if (Overlay_Dialogue && Text_DialogueSpeaker && Text_DialogueContent)
	{
		Overlay_Dialogue->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Text_DialogueSpeaker->SetText(FText::FromString(Speaker));
		Text_DialogueContent->SetText(FText::FromString(Text));
	}
}

void UExplorationUIWidget::ShowRegionName(const FString& RegionName)
{
	if (Text_RegionName)
	{
		Text_RegionName->SetText(FText::FromString(RegionName));
		PlayRegionNameAnimation(); // BP 애니메이션 킥
	}
}

void UExplorationUIWidget::AddPartyChat(const FPartyChatMsg& Msg)
{
	if (!VBox_PartyChat || !ChatBubbleClass) return;

	if (VBox_PartyChat->GetChildrenCount() >= 5)
	{
		if (UPartyChatBubbleWidget* OldestBubble = Cast<UPartyChatBubbleWidget>(VBox_PartyChat->GetChildAt(0)))
		{
			OldestBubble->ForceDismiss();
		}
	}

	UPartyChatBubbleWidget* NewBubble = CreateWidget<UPartyChatBubbleWidget>(this, ChatBubbleClass);
	if (NewBubble)
	{
		NewBubble->InitChatMessage(Msg);
		VBox_PartyChat->AddChildToVerticalBox(NewBubble);
	}
}

void UExplorationUIWidget::PlayPartyStatusAnim(bool bIsIntro)
{
	if (Anim_PartyStatusIntro && bIsIntro)
	{
		PlayAnimation(Anim_PartyStatusIntro);
	}
	else if (Anim_PartyStatusOuttro && !bIsIntro)
	{
		PlayAnimation(Anim_PartyStatusOuttro);
	}
}
