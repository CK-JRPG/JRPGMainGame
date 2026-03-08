#pragma once

#include"CoreMinimal.h"
#include"UObject/Interface.h"
#include"Combat/Characters/CombatTeamTypes.h"
#include"CombatParticipantInterface.generated.h"

class UHPComponent;
class UAPComponent;
class USPComponent;
class UActorComponent;

UINTERFACE(MinimalAPI)
class UCombatParticipantInterface : public UInterface
{
	GENERATED_BODY()
};

class ICombatParticipantInterface
{
	GENERATED_BODY()

public:
	virtual FName GetCombatantId() const = 0;// CharacterId
	virtual ECombatTeam GetCombatTeam() const = 0;

	virtual bool IsPlayerControlledCombatant() const = 0;

	virtual UHPComponent* GetHP() const = 0;
	virtual UAPComponent* GetAP() const = 0;
	virtual USPComponent* GetSP() const = 0;

	// 확장: Skill/Threat/Groggy/Status 등은 “있으면 반환” 패턴 권장
	virtual UActorComponent* GetOptionalComponentByClass(TSubclassOf<UActorComponent>CompClass) const = 0;
};