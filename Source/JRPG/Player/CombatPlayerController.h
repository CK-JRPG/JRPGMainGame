#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CombatPlayerController.generated.h"

UCLASS()
class JRPG_API ACombatPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	int32 SelectedPartyIndex = 0;

	void EnsureUI();
	void ApplySelectedIndexToWidgets() const;

	void ToggleTactical();

	void SelectParty1();
	void SelectParty2();
	void SelectParty3();

	void ReserveSlot1();
	void ReserveSlot2();
	void ReserveSlot3();
	void ReserveSlot4();
	void ClearReservation();

	void StartChain();
	void ConfirmChain();
	void CancelChain();

	void ReserveSlot(int32 Slot);
	void PickChainSlot(int32 Slot);
	
	// 플레이어 이동
	void MoveForward(float Value);
	void MoveRight(float Value);
	
	TArray<AActor*> GetParty() const;
	AActor* GetPartyMember(int32 Index) const;
	AActor* GetCurrentChainTarget() const;
};
