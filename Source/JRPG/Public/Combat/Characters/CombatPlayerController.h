#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CombatPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 전투 전용 플레이어 컨트롤러
 * 인카운터 진입 시 JRPGPlayerController 대신 CombatCharacterActor를 조작
 */
UCLASS()
class JRPG_API ACombatPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Combat;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Move;     // Value: Vector2D

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Look;

	// 임시로 Q, E로 바인딩 했음.
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_SwitchPrev;   // Q : 이전 파티원으로 전환

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_SwitchNext;   // E : 다음 파티원으로 전환

private:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnSwitchPrev(const FInputActionValue& Value);
	void OnSwitchNext(const FInputActionValue& Value);
	void SwitchCombatCharacter(int32 Direction);
	void UpdateCameraTargetForPawn(APawn* InPawn) const;
};