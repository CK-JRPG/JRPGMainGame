#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "CombatHUDPresenter.generated.h"

UCLASS()
class JRPG_API UCombatHUDPresenter : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(UWorld* InWorld, TSubclassOf<class UCombatUIWidget> WidgetClass);
	void ShutDown();

private:
	UPROPERTY() TObjectPtr<class UCombatUIWidget> CombatWidget;
	UPROPERTY() TObjectPtr<class UActionPaletteViewModel> ActionPaletteVM;
	UPROPERTY() TObjectPtr<class UEnemyViewModel> TargetVM;
	UPROPERTY() TArray<TObjectPtr<class UCombatPartySlotViewModel>> PartyVMs;
	UPROPERTY() TArray<TObjectPtr<class UEnemyViewModel>> EnemyHPBVarVMs;

	void OnBattleStarted(const FBattleSessionSnapshot* Snapshot);
	void OnBattleEnded(const FBattleSessionSnapshot* Snapshot, EBattleEndReason Reason);

	void OnActionPaletteSPUpdated(float Percent, const FString& Text);
	void OnTargetNameUpdated(const FString& Name);
	void OnTargetHPUpdated(float Percent, const FString& Text);
	void OnTargetGroggyUpdated(bool bGroggy);

	void OnPartySlotNameUpdated(class UCombatPartySlotWidget* View, const FString& Name);
	void OnPartySlotHPUpdated(class UCombatPartySlotWidget* View, float Percent, const FString& Text);
	void OnPartySlotSPUpdated(class UCombatPartySlotWidget* View, float Percent);

	void OnEnemyHPBarUpdated(class UEnemyHPBarWidget* View, float Percent, const FString& Text);
};
