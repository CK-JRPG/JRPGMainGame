#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "CombatCharacterActor.generated.h"

class UCombatCharacterComponent;
class UCombatStatsComponent;
class UCombatActionComponent;
class UJRPGSkillComponent;
class UStatusEffectComponent;
class UGroggyComponent;
class UCombatThreatComponent;
class UCombatAIActionSelectorComponent;
class UCombatItemComponent;
class UCombatPresentationComponent;
class UJRPGCombatMotionComponent;

class UCombatHPComponent;
class UCombatAPComponent;
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
	UPROPERTY(VisibleAnywhere) TObjectPtr<UJRPGSkillComponent> SkillComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStatusEffectComponent> StatusComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UGroggyComponent> GroggyComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatThreatComponent> ThreatComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatAIActionSelectorComponent> AIActionSelectorComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatItemComponent> ItemComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatPresentationComponent> PresentationComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UJRPGCombatMotionComponent> MotionComp;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatHPComponent> HPComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatAPComponent> APComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USPComponent> SPComp;

	virtual FName GetCombatantId() const override;
	virtual ECombatTeam GetCombatTeam() const override;
	virtual bool IsPlayerControlledCombatant() const override;

	virtual UCombatHPComponent* GetHP() const override { return HPComp; }
	virtual UCombatAPComponent* GetAP() const override { return APComp; }
	virtual USPComponent* GetSP() const override { return SPComp; }

	virtual UActorComponent* GetOptionalComponentByClass(TSubclassOf<UActorComponent> CompClass) const override;

protected:

	virtual void BeginPlay() override;
};
