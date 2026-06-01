#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimMontage.h"
#include "Combat/Core/RoleTypes.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Combat/Motion/CombatMotionTypes.h"
#include "CombatCharacterDataAsset.generated.h"

class USkillDataAsset;
class UNiagaraSystem;
class UCurveFloat;

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
	UPROPERTY(EditAnywhere) UTexture2D* Portrait;

	UPROPERTY(EditAnywhere) ECombatTeam DefaultTeam = ECombatTeam::Player;
	UPROPERTY(EditAnywhere) EJRPGPartyRole DefaultRole = EJRPGPartyRole::Attacker;
	
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
	UPROPERTY(EditAnywhere, Category="Basic Attack|VFX") TObjectPtr<UNiagaraSystem> BasicAttackHitNiagaraEffect = nullptr;
	UPROPERTY(EditAnywhere, Category="Basic Attack|VFX") TObjectPtr<UCurveFloat> BasicAttackDamageToEffectScaleCurve = nullptr;
	UPROPERTY(EditAnywhere, Category="Basic Attack|VFX", meta=(ClampMin="0.0")) float BasicAttackEffectScaleReferenceDamage = 100.f;
	UPROPERTY(EditAnywhere, Category="Basic Attack|VFX", meta=(ClampMin="0.0")) float BasicAttackEffectScaleMultiplier = 0.25f;
	UPROPERTY(EditAnywhere, Category="Basic Attack|VFX", meta=(ClampMin="1.0")) float BasicAttackMinEffectScale = 1.f;
	UPROPERTY(EditAnywhere, Category="Basic Attack|VFX", meta=(ClampMin="1.0")) float BasicAttackMaxEffectScale = 3.f;
	
	UPROPERTY(EditAnywhere, Category="Basic Attack|Camera")
	FCombatCameraShakeSpec BasicAttackCameraShake;

	// 스킬 ID만 보관 (에디터에서 KnownSkills에 이미 DA가 있을 때 해금 용도)
	UPROPERTY(EditAnywhere) TArray<FName> StartingSkillIds;
	// DA 직접 레퍼런스 (이 배열에 넣으면 BeginPlay 시 자동으로 LearnSkill 호출)
	UPROPERTY(EditAnywhere) TArray<TObjectPtr<USkillDataAsset>> StartingSkills;
	UPROPERTY(EditAnywhere) TMap<FName,int32> StartingItems;

	UPROPERTY(EditAnywhere) bool bHasBasicAttackMotion = false;
	UPROPERTY(EditAnywhere) FJRPGCombatMotionRequest BasicAttackMotion;

	// AI 이동/사거리 파라미터


	// true이면 원거리 캐릭터 (공격 사거리 밖에서 공격 가능, 거리 유지)
	UPROPERTY(EditAnywhere, Category = "AI") bool bIsRangedCombatant = false;
	// 기본 공격 최대 사거리 
	UPROPERTY(EditAnywhere, Category = "AI") float AttackRange = 200.f;
	// 원거리 캐릭터의 최소 거리
	UPROPERTY(EditAnywhere, Category = "AI") float PreferredMinRange = 0.f;
	/// 타겟이 이 거리 이상 벗어나면 Chase 시작 (원거리 전용)
	UPROPERTY(EditAnywhere, Category = "AI") float ChaseLeashRange = 1200.f;
	
	bool IsValidDef() const { return !CharacterId.IsNone(); }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FName("CombatCharacterData"), CharacterId);
	}
};
