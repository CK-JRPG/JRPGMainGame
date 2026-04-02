#include "UI/Presenters/CombatHUDPresenter.h"

#include "UI/Combat/CombatUIWidget.h"
#include "UI/Combat/CombatActionPaletteWidget.h"
#include "UI/Combat/CombatTargetInfoWidget.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "UI/Combat/EnemyHPBarWidget.h"
#include "UI/ViewModels/CombatViewModels.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"

#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

void UCombatHUDPresenter::OnBattleStarted(const FBattleSessionSnapshot* Snapshot)
{
}

void UCombatHUDPresenter::ShutDown()
{
}

void UCombatHUDPresenter::OnBattleEnded(const FBattleSessionSnapshot* Snapshot, EBattleEndReason Reason)
{
}

void UCombatHUDPresenter::OnActionPaletteSPUpdated(float Percent, const FString& Text)
{
}

void UCombatHUDPresenter::OnTargetNameUpdated(const FString& Name)
{
}

void UCombatHUDPresenter::OnTargetHPUpdated(float Percent, const FString& Text)
{
}

void UCombatHUDPresenter::OnTargetGroggyUpdated(bool bGroggy)
{
}

void UCombatHUDPresenter::OnPartySlotNameUpdated(UCombatPartySlotWidget* View, const FString& Name)
{
}

void UCombatHUDPresenter::OnPartySlotHPUpdated(UCombatPartySlotWidget* View, float Percent, const FString& Text)
{
}

void UCombatHUDPresenter::OnPartySlotSPUpdated(UCombatPartySlotWidget* View, float Percent)
{
}

void UCombatHUDPresenter::OnEnemyHPBarUpdated(UEnemyHPBarWidget* View, float Percent, const FString& Text)
{
}
