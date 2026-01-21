#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;
class UThreatComponent;
class USkillComponent;
class UCombatAIComponent;

UCLASS()
class JRPG_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
    AEnemyCharacter();

    // ====== ÄÄÆ÷³ÍÆ® ======
    UPROPERTY(VisibleAnywhere, Category = "Components|HP") TObjectPtr<UHealthComponent> Health;
    UPROPERTY(VisibleAnywhere, Category = "Components|AP") TObjectPtr<UThreatComponent> Threat;
    UPROPERTY(VisibleAnywhere, Category = "Components|Skill") TObjectPtr<USkillComponent> Skills;
    UPROPERTY(VisibleAnywhere, Category = "Components|AI") TObjectPtr<UCombatAIComponent> CombatAI;

protected:
    virtual void BeginPlay() override;
};
