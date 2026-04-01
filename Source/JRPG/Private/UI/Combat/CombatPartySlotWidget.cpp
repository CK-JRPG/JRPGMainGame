#include "UI/Combat/CombatPartySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"

void UCombatPartySlotWidget::BindPartyMember(AActor* MemberActor)
{
	if (!MemberActor) return;
    
	if (Text_Name) Text_Name->SetText(FText::FromString(MemberActor->GetName()));

	if (UHPComponent* HPComp = MemberActor->FindComponentByClass<UHPComponent>())
	{
		CachedHPComp = HPComp;
		HPComp->OnHPChanged.AddUObject(this, &UCombatPartySlotWidget::OnHPChanged);
        
		OnHPChanged(HPComp->GetHP(), HPComp->GetHP(), NAME_None);
	}

	if (UAPComponent* APComp = MemberActor->FindComponentByClass<UAPComponent>())
	{
		CachedAPComp = APComp;
		APComp->OnAPChanged.AddUObject(this, &UCombatPartySlotWidget::OnAPChanged);

		OnAPChanged(APComp->GetAP(), APComp->GetAP(), NAME_None);
	}
}

void UCombatPartySlotWidget::NativeDestruct()
{
	if (CachedHPComp.IsValid()) CachedHPComp->OnHPChanged.RemoveAll(this);
	if (CachedAPComp.IsValid()) CachedAPComp->OnAPChanged.RemoveAll(this);

	Super::NativeDestruct();
}

void UCombatPartySlotWidget::OnHPChanged(float OldHP, float NewHP, FName Reason)
{
	if (CachedHPComp.IsValid() && PB_HPBar && CachedHPComp->MaxHP > 0.f)
	{
		PB_HPBar->SetPercent(NewHP / CachedHPComp->MaxHP);
		Text_HP->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(NewHP), FMath::RoundToInt(CachedHPComp->MaxHP))));
	}
}

void UCombatPartySlotWidget::OnAPChanged(int32 OldAP, int32 NewAP, FName Reason)
{
	if (CachedAPComp.IsValid() && PB_APBar && CachedAPComp->MaxAP > 0.f)
	{
		PB_APBar->SetPercent(NewAP / CachedAPComp->MaxAP);
	}
}