#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CombatPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ULocalPlayer;
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
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Look;
	
	// 카메라 줌 In/Out, 키보드 : 마우스 휠 / 패드 : R1 + R_Stick
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_CameraZoom;
	
	// 적 포커싱, 키보드 : 마우스 휠 클릭 / 패드 : LStick 클릭 
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_TargetLockOn;

	// 임시로 Q, E로 바인딩 했음.
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_SwitchPrev;   // Q : 이전 파티원으로 전환

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_SwitchNext;   // E : 다음 파티원으로 전환
	
	// 전술 모드 진입 Tab
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_TacticalMode;

	// 메인 메뉴 토글
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ToggleMainMenu;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Attack;
	
	// 스킬 1 발동, 키보드 : 1
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Skill1;

private:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnSwitchPrev(const FInputActionValue& Value);
	void OnSwitchNext(const FInputActionValue& Value);
	void SwitchCombatCharacter(int32 Direction);
	void OnCameraZoom(const FInputActionValue& Value);
	void OnTargetLockOn(const FInputActionValue& Value);
	void UpdateCameraTargetForPawn(APawn* InPawn) const;
	void OnTacticalModePressed(const FInputActionValue& Value);
	void OnToggleMainMenu(const FInputActionValue& Value);
	void OnBasicAttackMouseClick(const FInputActionValue& Value);
	void OnSkill1(const FInputActionValue& Value);
};