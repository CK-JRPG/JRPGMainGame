#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "Components/PanelWidget.h"

void UCombatPartyRosterWidget::InitializeParty(AActor* PlayerActor)
{
	if (!PartyListContainer || !PartySlotClass) return;

	PartyListContainer->ClearChildren();

	// 추후 PartySubsystem에서 멤버 배열을 가져와서 반복문으로 생성하도록 수정
	// 현재는 플레이어 1명만 임시 등록합니다.
	UCombatPartySlotWidget* NewSlot = CreateWidget<UCombatPartySlotWidget>(GetWorld(), PartySlotClass);
	if (NewSlot)
	{
		NewSlot->BindPartyMember(PlayerActor);
		PartyListContainer->AddChild(NewSlot);
	}
}