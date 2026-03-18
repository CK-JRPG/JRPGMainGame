#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JRPGPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UCombatCharacterDataAsset;
struct FInputActionValue;

//해당 구조체 방식이 맞는가에 대해 점검중.
USTRUCT(BlueprintType)
struct FCharacterMappingRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCombatCharacterDataAsset> CharacterAsset;
};

UCLASS()
class JRPG_API AJRPGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Default;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Move;     // Value: Vector2D

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Sprint;   // Value: bool or Trigger

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Look;
	
	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Combat")
	TObjectPtr<UDataTable> CharacterTable;
	
	UPROPERTY(EditDefaultsOnly, Category="JRPG|Combat")
	TArray<FName> DefaultPartyIds;

	void OnMove(const FInputActionValue& Value);
	void OnSprintStarted(const FInputActionValue& Value);
	void OnSprintCompleted(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	
private:
	//테스트 중 - CombatCharacterActor와 JRPGPlayerPawn의 브릿지 함수
	void EnsureDefaultPartyFromTable();
	void InitallizeCombatBridge();
	UCombatCharacterDataAsset* FindCharacterDefById(FName CharId) const;
	
};
