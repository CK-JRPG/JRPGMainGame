#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimMontage.h"
#include "Combat/Core/RoleTypes.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Combat/Motion/CombatMotionTypes.h"
#include "CombatCharacterDataAsset.generated.h"

USTRUCT()
struct FCharacterBaseParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float BaseAttack =10.f;
	UPROPERTY(EditAnywhere) float BaseDefense =5.f;
	UPROPERTY(EditAnywhere) float BaseSpeed =10.f;

	UPROPERTY(EditAnywhere) float MaxHP =100.f;
	UPROPERTY(EditAnywhere) int32 MaxAP =10;
	UPROPERTY(EditAnywhere) int32 MaxSP =100;
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

	// 기본 공격 정의
	UPROPERTY(EditAnywhere) float BasicAttackBasePower = 5.f;
	UPROPERTY(EditAnywhere) float BasicAttackAttackScale = 1.0f;
	UPROPERTY(EditAnywhere) float BasicAttackDefenseScale = 0.5f;
	UPROPERTY(EditAnywhere) int32 BasicAttackAPCost = 0;
	UPROPERTY(EditAnywhere) int32 BasicAttackSPGainOnHit = 5;
	UPROPERTY(EditAnywhere) int32 BasicAttackSPGainOnKill = 10;
	UPROPERTY(EditAnywhere) float BasicAttackGroggyPower = 8.f;
	UPROPERTY(EditAnywhere) float BasicAttackThreatMultiplier = 1.0f;

	// 기본 공격 연출
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimMontage> BasicAttackMontage = nullptr;
	UPROPERTY(EditAnywhere) ECombatResolveTiming BasicAttackResolveTiming = ECombatResolveTiming::AnimNotifyWindow;
	UPROPERTY(EditAnywhere) FName BasicAttackStartCueTag = "Attack.Start";
	UPROPERTY(EditAnywhere) FName BasicAttackHitCueTag = "Attack.Hit";
	UPROPERTY(EditAnywhere) FName BasicAttackFinishCueTag = "Attack.Finish";

	UPROPERTY(EditAnywhere) TArray<FName> StartingSkillIds;
	UPROPERTY(EditAnywhere) TMap<FName,int32> StartingItems;

	UPROPERTY(EditAnywhere) bool bHasBasicAttackMotion = false;
	UPROPERTY(EditAnywhere) FCombatMotionRequest BasicAttackMotion;
	
	bool IsValidDef() const { return !CharacterId.IsNone(); }
};