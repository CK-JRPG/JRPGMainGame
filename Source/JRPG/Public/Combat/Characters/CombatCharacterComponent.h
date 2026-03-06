#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Core/RoleTypes.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "CombatCharacterComponent.generated.h"

class UCombatCharacterDataAsset;
class UHPComponent;
class UAPComponent;
class USPComponent;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatCharacterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatCharacterComponent();

	UPROPERTY(EditAnywhere) TObjectPtr<UCombatCharacterDataAsset> CharacterDef = nullptr;

	UPROPERTY(VisibleAnywhere) FName CharacterId = NAME_None;
	UPROPERTY(VisibleAnywhere) ECombatTeam Team = ECombatTeam::Player;
	UPROPERTY(VisibleAnywhere) EPartyRole Role = EPartyRole::Attacker;

	// 핵심 리소스(필수)
	UPROPERTY(Transient) TObjectPtr<UHPComponent> HP = nullptr;
	UPROPERTY(Transient) TObjectPtr<UAPComponent> AP = nullptr;
	UPROPERTY(Transient) TObjectPtr<USPComponent> SP = nullptr;

	FName GetCharacterId() const { return CharacterId; }
	ECombatTeam GetTeam() const { return Team; }
	EPartyRole GetRole() const { return Role; }

	// 초기화(Def 기반)
	void InitializeFromDef();

protected:
	virtual void BeginPlay()override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RegisterToRegistry();
	void UnregisterFromRegistry();

	void GiveStartingItems();
	void ApplyBaseParamsToResources();
};