#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatPartyRosterWidget.generated.h"

class ACombatCharacterActor;

UCLASS()
class JRPG_API UCombatPartyRosterWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// 에디터에서 개별 파티원 슬롯 디자인을 지정하기 위한 변수
	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<class UCombatPartySlotWidget> PartySlotClass;
	
	void InitializePartyFromActors(const TArray<ACombatCharacterActor*>& PartyActors);
protected:
	// UMG에서 파티원 슬롯이 들어갈 빈 패널(VerticalBox 등)
	UPROPERTY(meta = (BindWidget))
	class UPanelWidget* PartyListContainer;
};