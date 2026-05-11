#include "UI/Presenters/CombatHUDPresenter.h"
#include "UI/Combat/CombatUIWidget.h"
#include "UI/Combat/CombatActionPaletteWidget.h"
#include "UI/Combat/CombatTargetInfoWidget.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "UI/Combat/EnemyHPBarWidget.h"
#include "UI/Combat/TacticalUIWidget.h"
#include "UI/ViewModels/CombatViewModels.h"
#include "UI/Combat/DamageTextWidget.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"
#include "Components/WidgetComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Combat/CombatTagSwapWidget.h"
#include "Combat/Characters/CombatTransitionSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Engine/AssetManager.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Skills/SkillDataAsset.h"

void UCombatHUDPresenter::Initialize(UWorld* InWorld, TSubclassOf<UCombatUIWidget> WidgetClass, TSubclassOf<UTacticalUIWidget> TacticalClass)
{
	if (!InWorld) return;

	if (WidgetClass)
	{
		CombatWidget = CreateWidget<UCombatUIWidget>(InWorld, WidgetClass);
		if (CombatWidget)
		{
			CombatWidget->AddToViewport(0);
			CombatWidget->SetVisibility(ESlateVisibility::Hidden);

			ActionPaletteVM = NewObject<UActionPaletteViewModel>(this);
			ActionPaletteVM->OnSPUIUpdated.AddUObject(this, &UCombatHUDPresenter::OnActionPaletteSPUpdated);
			ActionPaletteVM->OnSkillListUpdated.AddUObject(this, &UCombatHUDPresenter::OnActionPaletteSkillUpdated);

			TargetVM = NewObject<UEnemyViewModel>(this);
			//TargetVM->OnTargetNameUpdated.AddUObject(this, &UCombatHUDPresenter::OnTargetNameUpdated);
			TargetVM->OnTargetHPUpdated.AddUObject(this, &UCombatHUDPresenter::OnTargetHPUpdated);
			//TargetVM->OnTargetGroggyUpdated.AddUObject(this, &UCombatHUDPresenter::OnTargetGroggyUpdated);
		}
	}

	if (TacticalClass)
	{
		TacticalWidget = CreateWidget<UTacticalUIWidget>(InWorld, TacticalClass);
		if (TacticalWidget)
		{
			TacticalWidget->AddToViewport(10);
			TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (UTacticalModeSubsystem* TacticalSub = InWorld->GetSubsystem<UTacticalModeSubsystem>())
	{
		TacticalSub->OnTacticalModeEntered.AddUObject(this, &UCombatHUDPresenter::OnTacticalModeEntered);
		TacticalSub->OnTacticalModeExited.AddUObject(this, &UCombatHUDPresenter::OnTacticalModeExited);
	}

	if (UBattleSessionSubsystem* BattleSub = InWorld->GetSubsystem<UBattleSessionSubsystem>())
	{
		BattleSub->OnBattleStarted.AddUObject(this, &UCombatHUDPresenter::OnBattleStarted);
		BattleSub->OnBattleEnded.AddUObject(this, &UCombatHUDPresenter::OnBattleEnded);

		if (BattleSub->IsBattleActive()) OnBattleStarted(FBattleSessionSnapshot());
	}

	if (UCombatTransitionSubsystem* TransitionSub = InWorld->GetSubsystem<UCombatTransitionSubsystem>())
	{
		TransitionSub->OnPartyMemberChangedDelegate.AddUObject(this, &UCombatHUDPresenter::OnActiveCharacterChanged);
	}
}

void UCombatHUDPresenter::Shutdown()
{
	if (ActionPaletteVM) ActionPaletteVM->Unbind();
	if (TargetVM) TargetVM->Unbind();
	for (auto& VM : PartyVMs) { if (VM) VM->Unbind(); }
	for (auto& VM : EnemyHPBarVMs) { if (VM) VM->Unbind(); }
	for (auto& VM : TagSwapVMs) { if (VM) VM->Unbind(); }

	if (UWorld* World = GetWorld())
	{
		if (UBattleSessionSubsystem* BattleSub = World->GetSubsystem<UBattleSessionSubsystem>())
		{
			BattleSub->OnBattleStarted.RemoveAll(this);
			BattleSub->OnBattleEnded.RemoveAll(this);
		}

		if (UTacticalModeSubsystem* TacticalSub = World->GetSubsystem<UTacticalModeSubsystem>())
		{
			TacticalSub->OnTacticalModeEntered.RemoveAll(this);
			TacticalSub->OnTacticalModeExited.RemoveAll(this);
		}

		if (UCombatTransitionSubsystem* TransitionSub = World->GetSubsystem<UCombatTransitionSubsystem>())
		{
			TransitionSub->OnPartyMemberChangedDelegate.RemoveAll(this);
		}
	}

	ClearHPBindings();

	if (CombatWidget) { CombatWidget->RemoveFromParent(); CombatWidget = nullptr; }
	if (TacticalWidget) { TacticalWidget->RemoveFromParent(); TacticalWidget = nullptr; }
	DamageTextPool.Empty();
}

void UCombatHUDPresenter::ShowDamageText(AActor* Target, float Damage, bool bIsCritical, EDamageTextType TextType)
{
	if (!CombatWidget || !DamageTextClass || !Target) return;
	UCanvasPanel* Canvas = CombatWidget->GetDamageCanvas();
	if (!Canvas) return;

	UDamageTextWidget* DmgWidget = nullptr;

	if (DamageTextPool.Num() > 0)
	{
		DmgWidget = DamageTextPool.Pop();
	}
	else
	{
		DmgWidget = CreateWidget<UDamageTextWidget>(GetWorld(), DamageTextClass);
		if (DmgWidget)
		{
			Canvas->AddChild(DmgWidget);
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(DmgWidget->Slot))
			{
				Slot->SetAlignment(FVector2D(0.5f, 0.5f));
				Slot->SetAutoSize(true);
			}

			DmgWidget->OnDamageTextFinished.BindUObject(this, &UCombatHUDPresenter::ReturnDamageTextToPool);
		}
	}

	if (DmgWidget)
	{
		DmgWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		DmgWidget->InitializeDamage(Target, Damage, bIsCritical, TextType);
	}
}

void UCombatHUDPresenter::OnActiveCharacterChanged(FName NewActiveID)
{
	if (!CombatWidget || !CombatWidget->ActionPalettePanel) return;

	UPartySubsystem* PartySys = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>();
	if (!PartySys) return;

	const TArray<FName>& PartyIds = PartySys->GetPartyIds();

	CombatWidget->ActionPalettePanel->ClearAllPartySlots();

	int32 SlotIndex = 0;
	for (UCombatPartySlotViewModel* VM : PartyVMs)
	{
		if (!VM) continue;
		FName CharID = VM->GetCharacterID();

		VM->OnNameUpdated.RemoveAll(this);
		VM->OnHPUIUpdated.RemoveAll(this);
		VM->OnAPUIUpdated.RemoveAll(this);

		if (UCombatPartySlotWidget* SlotWidget = CombatWidget->ActionPalettePanel->GetPartySlot(SlotIndex))
		{
			SlotWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SlotWidget->SetCharacterID(CharID);

			bool bIsActive = (CharID == NewActiveID);
			SlotWidget->SetIsActiveCharacter(bIsActive);

			VM->OnNameUpdated.AddUObject(this, &UCombatHUDPresenter::OnPartySlotNameUpdated, SlotWidget);
			VM->OnHPUIUpdated.AddUObject(this, &UCombatHUDPresenter::OnPartySlotHPUpdated, SlotWidget);
			VM->OnAPUIUpdated.AddUObject(this, &UCombatHUDPresenter::OnPartySlotAPUpdated, SlotWidget);

			SlotIndex++;
		}

		if (CharID == NewActiveID)
		{
			CurrentActivePartyVM = VM;

			VM->OnAPUIUpdated.AddUObject(this, &UCombatHUDPresenter::OnActionPaletteAPUpdated);
		}

		VM->Refresh();
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
		if (ActionPaletteVM) ActionPaletteVM->BindToPlayer(PlayerPawn);
	}

	if (UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>())
	{
		if (ACombatCharacterActor* ActiveActor = SpawnSub->FindActorByCharacterID(NewActiveID))
		{
			if (UCombatPresentationComponent* PC = ActiveActor->FindComponentByClass<UCombatPresentationComponent>())
			{
				PC->OnPresentationStarted.RemoveAll(this);
				PC->OnPresentationStarted.AddUObject(this, &UCombatHUDPresenter::OnCombatPresentationStarted);
			}
		}
	}
}

void UCombatHUDPresenter::ReturnDamageTextToPool(UDamageTextWidget* Widget)
{
	if (Widget)
	{
		DamageTextPool.Add(Widget);
	}
}

void UCombatHUDPresenter::OnBattleStarted(const FBattleSessionSnapshot& Snapshot)
{
	if (!CombatWidget) return;
	CombatWidget->SetVisibility(ESlateVisibility::Hidden);
	CombatWidget->SetCombatPanelsVisible(false);
	CombatWidget->HideEncounterOverlay();
	UE_LOG(LogTemp, Warning, TEXT("UCombatHUDPresenter::OnBattleStarted"));

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
		if (ActionPaletteVM) ActionPaletteVM->BindToPlayer(PlayerPawn);
	}

	PartySlotWidgets.Empty();
	for (auto& VM : PartyVMs) { if (VM) VM->Unbind(); }
	PartyVMs.Empty();

	TArray<FName> PartyIdsForUI;
	if (UPartySubsystem* PartySys = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
	{
		PartyIdsForUI = PartySys->GetPartyIds();
		for (FName CharID : PartyIdsForUI)
		{
			UCombatPartySlotViewModel* SlotVM = NewObject<UCombatPartySlotViewModel>(this);
			SlotVM->BindToCharacter(CharID);
			PartyVMs.Add(SlotVM);

			//if (CombatWidget->PartyRosterPanel && CombatWidget->PartyRosterPanel->PartySlotClass)
			//{
			//	UCombatPartySlotWidget* SlotWidget = CreateWidget<UCombatPartySlotWidget>(GetWorld(), CombatWidget->PartyRosterPanel->PartySlotClass);
			//	PartySlotWidgets.Add(CharID, SlotWidget);
			//}
		}
	}

	ClearHPBindings();

	if (UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>())
	{
		for (int32 i = 0; i < PartyIdsForUI.Num(); ++i)
		{
			FName CharID = PartyIdsForUI[i];
			ACombatCharacterActor* Actor = SpawnSub->FindActorByCharacterID(PartyIdsForUI[i]);
			if (!Actor) continue;

			if (PartyVMs.IsValidIndex(i))
			{
				PartyVMs[i]->BindToActor(Actor);
			}

			if (UHPComponent* HPComp = Actor->FindComponentByClass<UHPComponent>()) {
				HPComp->OnHPChanged.AddUObject(this, &UCombatHUDPresenter::HandleActorHPChangedForDamageText, Cast<AActor>(Actor));
				BoundHPComps.Add(HPComp);
			}

			if (USkillComponent* SkillComp = Actor->FindComponentByClass<USkillComponent>())
			{
				SkillComp->OnSkillCooldownFinished.RemoveAll(this);
				SkillComp->OnSkillCooldownFinished.AddUObject(this, &UCombatHUDPresenter::HandleSkillCooldownFinished, CharID);
			}
		}
	}

	// 적군 세팅 (메인 타겟팅 & 머리 위 HP바 연동)
	for (auto& VM : EnemyHPBarVMs) { if (VM) VM->Unbind(); }
	EnemyHPBarVMs.Empty();

	if (UBattleSessionSubsystem* BattleSub = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		TArray<AActor*> ActiveEnemies;
		BattleSub->GetAliveParticipantsByTeam(ECombatTeam::Enemy, ActiveEnemies);

		if (ActiveEnemies.Num() > 0 && TargetVM) {
			TargetVM->BindToEnemy(ActiveEnemies[0]);
		}

		for (AActor* Enemy : ActiveEnemies)
		{
			if (UWidgetComponent* HPBarComp = Enemy->FindComponentByClass<UWidgetComponent>())
			{
				HPBarComp->SetVisibility(true);
				if (UEnemyHPBarWidget* HPWidget = Cast<UEnemyHPBarWidget>(HPBarComp->GetUserWidgetObject()))
				{
					UEnemyViewModel* EnemyVM = NewObject<UEnemyViewModel>(this);
					EnemyVM->OnTargetHPUpdated.AddUObject(this, &UCombatHUDPresenter::OnEnemyHPBarUpdated, HPWidget);
					EnemyVM->BindToEnemy(Enemy);
					EnemyHPBarVMs.Add(EnemyVM);
				}
			}

			if (UHPComponent* HPComp = Enemy->FindComponentByClass<UHPComponent>()) {
				HPComp->OnHPChanged.AddUObject(this, &UCombatHUDPresenter::HandleActorHPChangedForDamageText, Enemy);
				BoundHPComps.Add(HPComp);
			}
		}
	}

	for (auto& VM : TagSwapVMs) { if (VM) VM->Unbind(); }
	TagSwapVMs.Empty();

	if (UPartySubsystem* PartySys = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
	{
		for (FName CharID : PartySys->GetPartyIds())
		{
			UCombatPartySlotViewModel* SwapVM = NewObject<UCombatPartySlotViewModel>(this);

			SwapVM->BindToCharacter(CharID);
			TagSwapVMs.Add(SwapVM);
		}
	}

	if (UCombatTransitionSubsystem* TransitionSub = GetWorld()->GetSubsystem<UCombatTransitionSubsystem>())
	{
		OnActiveCharacterChanged(TransitionSub->GetCurrentPlayerCharacterID());
	}
}

void UCombatHUDPresenter::OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason)
{
	if (CombatWidget)
	{
		CombatWidget->HideEncounterOverlay();
		CombatWidget->SetCombatPanelsVisible(false);
		CombatWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	if (ActionPaletteVM) ActionPaletteVM->Unbind();
	if (TargetVM) TargetVM->Unbind();
	for (auto& VM : PartyVMs) { if (VM) VM->Unbind(); }
	for (auto& VM : EnemyHPBarVMs) { if (VM) VM->Unbind(); }
	ClearHPBindings();
}

void UCombatHUDPresenter::ClearHPBindings()
{
	for (TWeakObjectPtr<UHPComponent> HPComp : BoundHPComps)
	{
		if (HPComp.IsValid())
		{
			HPComp->OnHPChanged.RemoveAll(this);
		}
	}
	BoundHPComps.Empty();
}

//void UCombatHUDPresenter::OnActionPaletteHPUpdated(float Percent, const FString& Text)
//{
//	if (CombatWidget && CombatWidget->ActionPalettePanel) 
//	    CombatWidget->ActionPalettePanel->UpdateHP(Percent, Text);
//}
//
void UCombatHUDPresenter::OnActionPaletteAPUpdated(float Percent)
{
	if (CombatWidget && CombatWidget->ActionPalettePanel) 
	    CombatWidget->ActionPalettePanel->UpdateAP(Percent);
}

void UCombatHUDPresenter::OnActionPaletteSkillUpdated(const TArray<FString>& SkillNames)
{
	if (CombatWidget && CombatWidget->ActionPalettePanel)
	{
		CombatWidget->ActionPalettePanel->UpdateSkillList(SkillNames);
	}
}

void UCombatHUDPresenter::OnCombatPresentationStarted(EPresentedCombatActionType ActionType, FName ActionId)
{
	if (ActionType == EPresentedCombatActionType::Skill)
	{
		if (CombatWidget)
		{
			FString DisplayName = ActionId.ToString();
			CombatWidget->PlaySkillAnnouncer(DisplayName);
		}
	}
}

void UCombatHUDPresenter::HandleSkillCooldownFinished(FName SkillId, FName CharacterID)
{
	if (!CombatWidget) return;

	FString CharName = CharacterID.ToString();
	FString SkillName = SkillId.ToString();
	UTexture2D* SkillIcon = nullptr;

	FPrimaryAssetId CharAssetId = FPrimaryAssetId(FName("CombatCharacterData"), CharacterID);
	if (UAssetManager* AssetMgr = UAssetManager::GetIfValid())
	{
		if (UObject* LoadedAsset = AssetMgr->GetPrimaryAssetObject(CharAssetId))
		{
			if (const UCombatCharacterDataAsset* CharDA = Cast<UCombatCharacterDataAsset>(LoadedAsset))
			{
				CharName = CharDA->DisplayName.ToString();
			}
		}
	}

	if (UPartyActorSpawnSubsystem* SpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>())
	{
		if (ACombatCharacterActor* Actor = SpawnSub->FindActorByCharacterID(CharacterID))
		{
			if (USkillComponent* SkillComp = Actor->FindComponentByClass<USkillComponent>())
			{
				if (USkillDataAsset* SkillDA = SkillComp->GetSkillDef(SkillId))
				{
					SkillName = SkillDA->DisplayName.ToString();
					//SkillIcon = SkillDA->KeyIcon;
				}
			}
		}
	}

	FString LogMessage = FString::Printf(TEXT("(%s의 %s) 준비 완료"), *CharName, *SkillName);

	CombatWidget->AddCombatLog(LogMessage, SkillIcon);
}

void UCombatHUDPresenter::OnTacticalModeEntered(const FTacticalModeSnapshot& Snapshot)
{
	if (TacticalWidget)
	{
		TacticalWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UCombatHUDPresenter::OnTacticalModeExited(const FTacticalModeSnapshot& Snapshot)
{
	if (TacticalWidget)
	{
		TacticalWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UCombatHUDPresenter::OnActionPaletteSPUpdated(float Percent, const FString& Text) {
	//if (CombatWidget && CombatWidget->ActionPalettePanel) CombatWidget->ActionPalettePanel->UpdateSPUI(Percent, Text);
}

//void UCombatHUDPresenter::OnTargetNameUpdated(const FString& Name) {
//	if (CombatWidget && CombatWidget->TargetInfoPanel) CombatWidget->TargetInfoPanel->UpdateTargetName(Name);
//}

void UCombatHUDPresenter::OnTargetHPUpdated(float Percent, const FString& Text) {
	if (CombatWidget && CombatWidget->TargetInfoPanel) CombatWidget->TargetInfoPanel->UpdateTargetHP(Percent);
}

//void UCombatHUDPresenter::OnTargetGroggyUpdated(bool bGroggy) {
//	if (CombatWidget && CombatWidget->TargetInfoPanel) CombatWidget->TargetInfoPanel->UpdateGroggyState(bGroggy);
//}

void UCombatHUDPresenter::OnPartySlotNameUpdated(const FString& Name, UCombatPartySlotWidget* View)
{
	if (View) View->UpdateName(Name);
}

void UCombatHUDPresenter::OnPartySlotHPUpdated(float Percent, const FString& Text, UCombatPartySlotWidget* View)
{
	if (View) View->UpdateHP(Percent, Text);
}

void UCombatHUDPresenter::OnPartySlotAPUpdated(float Percent, UCombatPartySlotWidget* View)
{
	if (View) View->UpdateAP(Percent);
}

void UCombatHUDPresenter::OnEnemyHPBarUpdated(float Percent, const FString& Text, UEnemyHPBarWidget* View)
{
	if (View) View->UpdateHP(Percent);
}

void UCombatHUDPresenter::HandleActorHPChangedForDamageText(float OldHP, float NewHP, FName Reason, AActor* TargetActor)
{
	if (!TargetActor) return;

	float Difference = NewHP - OldHP;

	if (FMath::IsNearlyZero(Difference)) return;

	bool bIsHeal = (Difference > 0.f);
	float AbsoluteAmount = FMath::Abs(Difference);

	bool bIsCritical = false;

	EDamageTextType TextType = EDamageTextType::EnemyDamage;

	if (bIsHeal)
	{
		TextType = EDamageTextType::Heal;
	}
	else
	{
		ICombatParticipantInterface* T = Cast<ICombatParticipantInterface>(TargetActor);

		if (!T) return;

		const ECombatTeam TT = T->GetCombatTeam();

		if (TT == ECombatTeam::Player)
		{
			TextType = EDamageTextType::PlayerDamage;
		}
	}

	ShowDamageText(TargetActor, AbsoluteAmount, bIsCritical, TextType);
}

UCombatPartySlotViewModel* UCombatHUDPresenter::GetPartySLotVM(FName CharID)
{
	for (UCombatPartySlotViewModel* VM : TagSwapVMs)
	{
		if (VM && VM->GetCharacterID() == CharID) return VM;
	}
	return nullptr;
}

void UCombatHUDPresenter::ShowSkillAnnouncer(const FString& SkillName)
{
	if (CombatWidget)
	{
		CombatWidget->PlaySkillAnnouncer(SkillName);
	}
}

void UCombatHUDPresenter::BeginEncounterIntro()
{
	if (!CombatWidget)
	{
		return;
	}

	CombatWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CombatWidget->SetCombatPanelsVisible(false);
	CombatWidget->ShowEncounterOverlay();
}

void UCombatHUDPresenter::EndEncounterIntro()
{
	if (!CombatWidget)
	{
		return;
	}

	CombatWidget->HideEncounterOverlay();
	CombatWidget->SetCombatPanelsVisible(true);
	CombatWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCombatHUDPresenter::UpdateTargetEnemyUI(AActor* NewTarget)
{
	UE_LOG(LogTemp, Warning, TEXT("UCombatHUDPresenter::UpdateTargetEnemyUI"));
	if (!TargetVM || !CombatWidget || !CombatWidget->TargetInfoPanel) return;

	if (NewTarget)
	{
		TargetVM->BindToEnemy(NewTarget);

		CombatWidget->TargetInfoPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		TargetVM->Unbind();
		CombatWidget->TargetInfoPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCombatHUDPresenter::SetPartyWheelActive(bool bActive)
{
	if (CombatWidget && CombatWidget->ActionPalettePanel)
	{
		CombatWidget->ActionPalettePanel->SetWheelModeActive(bActive);
	}
}

FName UCombatHUDPresenter::GetHoveredPartyMemberID() const
{
	if (CombatWidget && CombatWidget->ActionPalettePanel)
	{
		return CombatWidget->ActionPalettePanel->GetSelectedPartyMemberID();
	}
	return NAME_None;
}
