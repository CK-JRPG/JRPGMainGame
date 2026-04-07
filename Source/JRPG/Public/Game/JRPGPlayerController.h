#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JRPGPlayerController.generated.h"

class AJRPGCompanionPawn;
class ACombatCharacterActor;
class UInputMappingContext;
class UInputAction;
class UCombatCharacterDataAsset;
struct FInputActionValue;

USTRUCT(BlueprintType)
struct FCharacterMappingRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCombatCharacterDataAsset> CharacterAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<ACombatCharacterActor> CombatActorClass;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<AJRPGCompanionPawn> FieldPawnClass;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SpawnOffset = FVector::ZeroVector;
};

UCLASS()
class JRPG_API AJRPGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Default;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Move;     // Value: Vector2D

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Sprint;   // Value: bool or Trigger

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Look;
	
	// 주변 둘러보기 키
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_LookAround;  // 임시 바인딩 마우스 버튼 5
	
	// 카메라 줌 In/Out 키
	// 키보드 : 마우스 휠 / 패드 : R1 + R_Stick
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_CameraZoom;

	// 메인메뉴 토글
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ToggleMainMenu;

	// 상호작용 키 - E
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Interact;
	
	// 카메라 설정
	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Camera")
	float LookSensitivityX = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Camera")
	float LookSensitivityY = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Combat")
	TObjectPtr<UDataTable> CharacterTable;
	
	UPROPERTY(EditDefaultsOnly, Category="JRPG|Combat")
	TArray<FName> DefaultPartyIds;

	// 전투 전용 PlayerController BP 클래스 (BP_CombatPlayerController 등)
	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Combat")
	TSubclassOf<APlayerController> CombatControllerClass;

	void OnMove(const FInputActionValue& Value);
	void OnSprintStarted(const FInputActionValue& Value);
	void OnSprintCompleted(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnLookAround();
	void OnLookAroundCompleted();
	void OnCameraZoom(const FInputActionValue& Value);
	void OnInteract();
	
	void UpdateCameraTargetForPawn(APawn* InPawn) const;
	
private:
	void EnsureDefaultPartyFromTable();
	void InitallizeCombatBridge();
	UCombatCharacterDataAsset* FindCharacterDefById(FName CharId) const;
	FCharacterMappingRow*      FindMappingRowById(FName CharId) const;

	void OnToggleMainMenu();

public:
	// 인벤토리 테스트를 위한 디버그/치트 명령어
	UFUNCTION(Exec, Category = "JRPG|Cheats")
	void GiveItem(FName ItemId, int32 Count = 1);

	UFUNCTION(Exec, Category = "JRPG|Cheats")
	void ClearInventory();

	UFUNCTION(Exec, Category = "JRPG|Cheats")
	void DumpInventory();

	UFUNCTION(Exec, Category = "JRPG|Cheats")
	void DumpEquipment(FName CharacterId = NAME_None);

	UFUNCTION(Exec, Category = "JRPG|Cheats")
	void ForceEquip(FName CharacterId, int32 Slot, FName ItemId);


	UFUNCTION(Exec, Category = "JRPG|Cheats")
	void RebuildStats(FName CharacterId = NAME_None);

	//UFUNCTION(Exec, Category = "JRPG|Cheats")
	//void AutoEquipForRole(FName CharacterId, FName Role);

private:
	AActor* GetCheatTargetActor(FName CharacterId) const;
};
