#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatCharacter.generated.h"

class UHealthComponent;
class UAPComponent;
class USkillComponent;
class UCombatAIComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class JRPG_API ACombatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACombatCharacter();

	UPROPERTY(EditAnywhere, Category="Combat") bool bIsPartyMember = true;

	// ====== 컴포넌트 ======
	UPROPERTY(VisibleAnywhere, Category="Components|HP") TObjectPtr<UHealthComponent> Health;
	UPROPERTY(VisibleAnywhere, Category="Components|AP") TObjectPtr<UAPComponent> AP;
	UPROPERTY(VisibleAnywhere, Category="Components|Skill") TObjectPtr<USkillComponent> Skills;
	UPROPERTY(VisibleAnywhere, Category="Components|AI") TObjectPtr<UCombatAIComponent> CombatAI;
	
	// ====== 카메라 ======
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<USpringArmComponent> SpringArm = nullptr;
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<UCameraComponent>    CombatCamera = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
