#include "UI/Presenters/InventoryPresenter.h"
#include "UI/ViewModels/InventoryViewModel.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/ItemDataAsset.h"
#include "Combat/Items/WeaponEquipComponent.h"
#include "Combat/Items/AugmentEquipComponent.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Items/ItemUseTypes.h"
#include "Combat/Characters/PartySubsystem.h"

void UInventoryPresenter::Initialize(UWorld* World, TSubclassOf<UUserWidget> MenuWidgetClass)
{
	CachedWorld = World;
	if (!World || !MenuWidgetClass) return;

	InventoryViewModel = NewObject<UInventoryViewModel>(this);
	InventoryViewModel->Initialize(World);

	MenuWidgetInstance = CreateWidget<UUserWidget>(World, MenuWidgetClass);
	if (MenuWidgetInstance)
	{
		UFunction* Func = MenuWidgetInstance->FindFunction(FName("SetViewModel"));
		if (Func)
		{
			struct { UInventoryViewModel* VM; } Params;
			Params.VM = InventoryViewModel;
			MenuWidgetInstance->ProcessEvent(Func, &Params);
		}

		MenuWidgetInstance->AddToViewport(50);
		MenuWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UInventoryPresenter::InitializeWithExistingWidget(UWorld* World, UUserWidget* InWidget)
{
	CachedWorld = World;
	if (!World || !InWidget) return;

	InventoryViewModel = NewObject<UInventoryViewModel>(this);
	InventoryViewModel->Initialize(World);

	MenuWidgetInstance = InWidget;

	UFunction* Func = MenuWidgetInstance->FindFunction(FName("SetViewModel"));
	if (Func)
	{
		struct { UInventoryViewModel* VM; } Params;
		Params.VM = InventoryViewModel;
		MenuWidgetInstance->ProcessEvent(Func, &Params);
	}
}

void UInventoryPresenter::OpenInventory(AActor* DefaultCharacter)
{
	if (!MenuWidgetInstance || !InventoryViewModel || bIsOpen) return;

	InventoryViewModel->SelectCharacter(DefaultCharacter);

	// 맨 처음 인벤토리를 열었을 때 무조건 장비 탭(0번)으로 초기화 & 정렬
	InventoryViewModel->FilterItems(EInventoryTab::Equipment);

	MenuWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bIsOpen = true;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(CachedWorld.Get(), 0))
	{
		PC->SetInputMode(FInputModeGameAndUI());
		PC->bShowMouseCursor = true;
	}
}

void UInventoryPresenter::CloseInventory()
{
	if (!MenuWidgetInstance || !InventoryViewModel || !bIsOpen) return;

	MenuWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	InventoryViewModel->ClearPreview();
	bIsOpen = false;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(CachedWorld.Get(), 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void UInventoryPresenter::HandleItemAction(FGuid InstanceId, AActor* TargetCharacter)
{
	if (!InstanceId.IsValid() || !TargetCharacter) return;

	UInventorySubsystem* InvSub = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>();
	if (!InvSub) return;

	FItemInstance Inst;
	if (!InvSub->TryGetInstance(InstanceId, Inst)) return;

	const UItemDataAsset* Def = InvSub->FindDef(Inst.ItemId);

	UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : 아이템 사용 시도"));

	// Weapon 장착 처리
	if (Def->IsWeapon())
	{
		UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : 무기 장착 시도"));
		if (UWeaponEquipComponent* WeaponComp = TargetCharacter->FindComponentByClass<UWeaponEquipComponent>())
		{
			FItemOp Op = WeaponComp->EquipWeapon(InvSub, InstanceId);
			if (!Op.bOk) UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : 무기 장착 실패 : %s"), *Op.ReasonTag.ToString());
			UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : 무기 장착 성공"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : WeaponComp is not invalid"));
		}
	}

	// Augment 장착 처리
	else if (Def->IsAugment())
	{
		UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : Augment 장착 시도"));
		if (UAugmentEquipComponent* AugmentComp = TargetCharacter->FindComponentByClass<UAugmentEquipComponent>())
		{
			EAugmentEquipSlot TargetSlot = EAugmentEquipSlot::AugmentSlot1;
			if(!AugmentComp->IsSlotOccupied(EAugmentEquipSlot::AugmentSlot1)) TargetSlot = EAugmentEquipSlot::AugmentSlot1;
			else if(!AugmentComp->IsSlotOccupied(EAugmentEquipSlot::AugmentSlot2)) TargetSlot = EAugmentEquipSlot::AugmentSlot2;
			else if(!AugmentComp->IsSlotOccupied(EAugmentEquipSlot::AugmentSlot3)) TargetSlot = EAugmentEquipSlot::AugmentSlot3;

			FItemOp Op = AugmentComp->TryEquipFromInventory(InvSub, InstanceId, TargetSlot);
			if (!Op.bOk) UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : 오그먼트 장착 실패 : %s"), *Op.ReasonTag.ToString());
			UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : Augment 장착 성공"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UInventoryPresenter::HandleItemAction : AugmentComp is not invalid"));
		}
	}

	// Consumable 소비 처리
	else if (Def->IsConsumable())
	{
		UCharacterRuntimeSubsystem* RuntimeSub = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterRuntimeSubsystem>();
		if (!RuntimeSub) return;

		FName TargetCharId = NAME_None;

		// 방법 A: PartySubsystem에서 첫 번째 멤버 ID 가져오기 (가장 권장되는 임시 방법)
		// ※ 서브시스템 이름과 배열 반환 함수명(GetPartyMembers)은 실제 프로젝트에 맞게 수정해주세요!
		if (UPartySubsystem* PartySub = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
		{
			TArray<FName> PartyList = PartySub->GetPartyIds();
			if (PartyList.Num() > 0)
			{
				TargetCharId = PartyList[0]; // 첫 번째 캐릭터 강제 지정
			}
		}

		if (TargetCharId.IsNone()) return;

		for (const FConsumableEffect& Effect : Def->UseEffects)
		{
			switch (Effect.Type)
			{
			case EConsumableEffectType::HealHPFlat:
				RuntimeSub->ModifyHP(TargetCharId, Effect.Value);
				break;

			case EConsumableEffectType::HealHPPctMax:
				if (const FCharacterResourceSnapshot* Snap = RuntimeSub->GetSnapshot(TargetCharId))
				{
					float HealAmount = Snap->MaxHP * Effect.Value;
					RuntimeSub->ModifyHP(TargetCharId, HealAmount);
				}
				break;

			case EConsumableEffectType::RestoreAPFlat:
				RuntimeSub->ModifyAP(TargetCharId, FMath::RoundToInt(Effect.Value));
				break;

			case EConsumableEffectType::GrantSPFlat:
				RuntimeSub->ModifySP(TargetCharId, FMath::RoundToInt(Effect.Value));
				break;

			case EConsumableEffectType::CleanseStatusId:
			case EConsumableEffectType::CleanseByTag:
			case EConsumableEffectType::ApplyStatusId:
				// TODO: 상태이상/버프 시스템 컴포넌트나 서브시스템이 만들어지면 연결
				UE_LOG(LogTemp, Warning, TEXT("상태이상(Status) 효과 적용: %s"), *Effect.StatusId.ToString());
				break;
			}
		}

		FItemOp Op = InvSub->RemoveItemByInstance(InstanceId, 1, TEXT("Consumable.Used"));
		if (!Op.bOk)
		{
			UE_LOG(LogTemp, Error, TEXT("소모품 사용 후 인벤토리 차감 실패: %s"), *Op.ReasonTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("소모품 사용 완료: %s (대상: %s)"), *Def->ItemId.ToString(), *TargetCharId.ToString());
		}
	}
}
