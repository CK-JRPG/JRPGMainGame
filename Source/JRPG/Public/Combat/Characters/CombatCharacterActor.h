#pragma once

#include "CoreMinimal.h"
#include "Combat/Camera/CameraTargetInterface.h"
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
class ULocomotionComponent;
class UEnemyEncounterComponent;
class UCombatZoneTrackerComponent;

class UHPComponent;
class UAPComponent;
class USPComponent;

UCLASS()
class JRPG_API ACombatCharacterActor :public ACharacter, public ICombatParticipantInterface, public ICameraTargetInterface
{
	GENERATED_BODY()

public:
	ACombatCharacterActor();

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
	UPROPERTY(VisibleAnywhere) TObjectPtr<ULocomotionComponent> LocomotionComp;
	
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHPComponent> HPComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAPComponent> APComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USPComponent> SPComp;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UEnemyEncounterComponent> EnemyEncounterComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCombatZoneTrackerComponent> ZoneTrackerComp;
	
	

	virtual FName GetCombatantId() const override;
	virtual ECombatTeam GetCombatTeam() const override;
	virtual bool IsPlayerControlledCombatant() const override;

	virtual UHPComponent* GetHP() const override { return HPComp; }
	virtual UAPComponent* GetAP() const override { return APComp; }
	virtual USPComponent* GetSP() const override { return SPComp; }

	virtual UActorComponent* GetOptionalComponentByClass(TSubclassOf<UActorComponent> CompClass) const override;

	
	virtual FVector  GetCameraTargetLocation()  const override;
	virtual FRotator GetCameraTargetRotation()  const override;
	virtual float    GetCameraTargetArmLength() const override;
	
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "100.0"))
	float CombatArmLength = 550.f;
	
protected:

	virtual void BeginPlay() override;
};
