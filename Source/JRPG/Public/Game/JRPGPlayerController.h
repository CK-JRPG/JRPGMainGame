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

//해당 구조체 방식이 맞는가에 대해 점검중.
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
	
	// 카메라 설정
	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Camera")
	float LookSensitivityX = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Camera")
	float LookSensitivityY = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "JRPG|Combat")
	TObjectPtr<UDataTable> CharacterTable;
	
	UPROPERTY(EditDefaultsOnly, Category="JRPG|Combat")
	TArray<FName> DefaultPartyIds;

	void OnMove(const FInputActionValue& Value);
	void OnSprintStarted(const FInputActionValue& Value);
	void OnSprintCompleted(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void UpdateCameraTargetForPawn(APawn* InPawn) const;
	
private:
	void EnsureDefaultPartyFromTable();
	void InitallizeCombatBridge();
	UCombatCharacterDataAsset* FindCharacterDefById(FName CharId) const;
	FCharacterMappingRow*      FindMappingRowById(FName CharId) const;

};
