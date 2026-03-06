#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Combat/Characters/CombatParticipantInterface.h"

#include "CombatCharacterActor.generated.h"

class UCombatCharacterComponent;
class UCombatLevelProviderComponent;
class UHPComponent;
class UAPComponent;
class USPComponent;

UCLASS()
class JRPG_API ACombatCharacter : public ACharacter, public ICombatParticipantInterface
{
	GENERATED_BODY()

public:
	ACombatCharacter();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatCharacterComponent> CharacterComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatLevelProviderComponent> LevelProviderComp;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UHPComponent> HPComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAPComponent> APComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USPComponent> SPComp;

	// ICombatParticipantInterface
	virtual FName GetCombatantId() const override;
	virtual ECombatTeam GetCombatTeam() const override;
	virtual bool IsPlayerControlledCombatant() const override;

	virtual UHPComponent* GetHP() const override { return HPComp; }
	virtual UAPComponent* GetAP() const override { return APComp; }
	virtual USPComponent* GetSP() const override { return SPComp; }

	virtual UActorComponent* GetOptionalComponentByClass(TSubclassOf<UActorComponent> CompClass) const override;

protected:
	virtual void BeginPlay()override;
};