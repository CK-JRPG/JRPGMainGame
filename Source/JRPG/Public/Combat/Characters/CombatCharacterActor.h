#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "CombatCharacterActor.generated.h"

class UCombatCharacterComponent;
class UCombatStatsComponent;
class UCombatActionComponent;
class USkillComponent;
class UStatusEffectComponent;
class UGroggyComponent;
class UThreatComponent;
class UCombatAIActionSelectorComponent;
class UCombatItemComponent;
class UCombatPresentationComponent;
class UCombatMotionComponent;

class UHPComponent;
class UAPComponent;
class USPComponent;

UCLASS()
class JRPG_API ACombatCharacter :public ACharacter, public ICombatParticipantInterface
{
	GENERATED_BODY()

public:
	ACombatCharacter();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatCharacterComponent> CharacterComp;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatStatsComponent> StatsComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatActionComponent> ActionComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkillComponent> SkillComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStatusEffectComponent> StatusComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UGroggyComponent> GroggyComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UThreatComponent> ThreatComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatAIActionSelectorComponent> AIActionSelectorComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatItemComponent> ItemComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatPresentationComponent> PresentationComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatMotionComponent> MotionComp;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UHPComponent> HPComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAPComponent> APComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USPComponent> SPComp;

	virtual FName GetCombatantId() const override;
	virtual ECombatTeam GetCombatTeam() const override;
	virtual bool IsPlayerControlledCombatant() const override;

	virtual UHPComponent* GetHP() const override { return HPComp; }
	virtual UAPComponent* GetAP() const override { return APComp; }
	virtual USPComponent* GetSP() const override { return SPComp; }

	virtual UActorComponent* GetOptionalComponentByClass(TSubclassOf<UActorComponent> CompClass) const override;

protected:

	virtual void BeginPlay() override;
};
