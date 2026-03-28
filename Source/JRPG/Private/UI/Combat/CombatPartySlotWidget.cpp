#include "UI/Combat/CombatPartySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Combat/Stats/HPComponent.h"

void UCombatPartySlotWidget::BindPartyMember(AActor* MemberActor)
{
	if (!MemberActor) return;
    
	if (Text_Name) Text_Name->SetText(FText::FromString(MemberActor->GetName()));

	if (UHPComponent* HPComp = MemberActor->FindComponentByClass<UHPComponent>())
	{
		CachedHPComp = HPComp;
		HPComp->OnHPChanged.AddUObject(this, &UCombatPartySlotWidget::OnHPChanged);
        
		// 최초 초기화 시 현재 HP를 세팅해 줍니다.
		OnHPChanged(HPComp->GetHP(), HPComp->GetHP(), NAME_None);
	}
}

void UCombatPartySlotWidget::NativeDestruct()
{
	if (CachedHPComp.IsValid()) CachedHPComp->OnHPChanged.RemoveAll(this);
	Super::NativeDestruct();
}

void UCombatPartySlotWidget::OnHPChanged(float OldHP, float NewHP, FName Reason)
{
	if (CachedHPComp.IsValid() && PB_MemberHP && CachedHPComp->MaxHP > 0.f)
	{
		PB_MemberHP->SetPercent(NewHP / CachedHPComp->MaxHP);
	}
}