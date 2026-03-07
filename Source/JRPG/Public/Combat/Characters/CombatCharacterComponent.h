#pragma once
#include"CoreMinimal.h"
#include"Components/ActorComponent.h"
#include"Combat/Core/RoleTypes.h"
#include"Combat/Characters/CombatTeamTypes.h"
#include"CombatCharacterComponent.generated.h"

class UCombatCharacterDataAsset;

UCLASS(ClassGroup=(Combat),meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatCharacterComponent :public UActorComponent
{
	GENERATED_BODY()
	
public:
	UCombatCharacterComponent();

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCombatCharacterDataAsset> CharacterDef = nullptr;

	UPROPERTY(VisibleAnywhere) FName CharacterId = NAME_None;
	UPROPERTY(VisibleAnywhere) ECombatTeam Team = ECombatTeam::Player;
	UPROPERTY(VisibleAnywhere) EPartyRole Role = EPartyRole::Attacker;

	FName GetCharacterId() const { return CharacterId; }
	ECombatTeam GetTeam() const { return Team; }
	EPartyRole GetRole() const { return Role; }

	void InitializeFromDef();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RegisterToRegistry();
	void UnregisterFromRegistry();

	void GiveStartingItems();
	void GiveStartingSkills();
};