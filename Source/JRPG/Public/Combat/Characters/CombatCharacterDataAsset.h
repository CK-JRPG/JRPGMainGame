#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Core/RoleTypes.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "CombatCharacterDataAsset.generated.h"

USTRUCT()
struct FCharacterBaseParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float BaseAttack = 10.f;
	UPROPERTY(EditAnywhere) float BaseDefense = 5.f;
	UPROPERTY(EditAnywhere) float BaseSpeed = 10.f;

	UPROPERTY(EditAnywhere) float MaxHP = 100.f;
	UPROPERTY(EditAnywhere) int32 MaxAP = 10;
	UPROPERTY(EditAnywhere) int32 MaxSP = 100;
};

UCLASS()
class JRPG_API UCombatCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) FName CharacterId = NAME_None;
	UPROPERTY(EditAnywhere) FText DisplayName;

	UPROPERTY(EditAnywhere) ECombatTeam DefaultTeam = ECombatTeam::Player;
	UPROPERTY(EditAnywhere) EPartyRole DefaultRole = EPartyRole::Attacker;

	UPROPERTY(EditAnywhere) FCharacterBaseParams BaseParams;

	// 스킬/아이템 연결
	UPROPERTY(EditAnywhere) TArray<FName> StartingSkillIds;
	UPROPERTY(EditAnywhere) TMap<FName, int32> StartingItems;

	bool IsValidDef() const { return !CharacterId.IsNone(); }
};