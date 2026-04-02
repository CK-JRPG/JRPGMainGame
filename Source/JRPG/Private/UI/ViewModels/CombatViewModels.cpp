#include "UI/ViewModels/CombatViewModels.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "GameFramework/Actor.h"

// --- Party Slot ViewModel ---
void UCombatPartySlotViewModel::BindToActor(AActor* MemberActor)
{
	Unbind();
	if (!MemberActor) return;
	OnNameUpdated.Broadcast(MemberActor->GetName());

	if (UHPComponent* HPComp = MemberActor->FindComponentByClass<UHPComponent>())
	{
		CachedHPComp = HPComp;
		HPComp->OnHPChanged.AddUObject(this, &UCombatPartySlotViewModel::HandleHPChanged);
		HandleHPChanged(HPComp->GetHP(), HPComp->GetHP(), NAME_None);
	}

	if (UAPComponent* APComp = MemberActor->FindComponentByClass<UAPComponent>())
	{
		CachedAPComp = APComp;
		APComp->OnAPChanged.AddUObject(this, &UCombatPartySlotViewModel::HandleAPChanged);
		HandleAPChanged(APComp->GetAP(), APComp->GetAP(), NAME_None);
	}
}

void UCombatPartySlotViewModel::Unbind()
{
	if (CachedHPComp.IsValid()) CachedHPComp->OnHPChanged.RemoveAll(this);
	if (CachedAPComp.IsValid()) CachedAPComp->OnAPChanged.RemoveAll(this);
}

void UCombatPartySlotViewModel::HandleHPChanged(float OldHP, float NewHP, FName Reason)
{
	if (CachedHPComp.IsValid())
	{
		float MaxHP = CachedHPComp->MaxHP;
		float Percent = MaxHP > 0.f ? NewHP / MaxHP : 0.f;
		FString Text = FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(NewHP), FMath::RoundToInt(MaxHP));
		OnHPUIUpdated.Broadcast(Percent, Text);
	}
}

void UCombatPartySlotViewModel::HandleAPChanged(int32 OldAP, int32 NewAP, FName Reason)
{
	if (CachedAPComp.IsValid())
	{
		float MaxAP = CachedAPComp->MaxAP;
		float Percent = MaxAP > 0.f ? (float)NewAP / MaxAP : 0.f;
		OnAPUIUpdated.Broadcast(Percent);
	}
}

// --- Enemy ViewModel --- 
void UEnemyViewModel::BindToEnemy(AActor* EnemyActor)
{
}


void UEnemyViewModel::HandleGroggyCHanged(bool bGroggy)
{
}


void UActionPalettedViewModel::BindToPlayer(AActor* PlayerActor)
{
}

void UActionPalettedViewModel::HandleSPChanged(int32 OldSP, int32 NewSP, FName Reason)
{
}
