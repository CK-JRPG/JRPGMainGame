#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPG/Public/Combat/Skills/JRPGSkillDataAsset.h"
#include "JRPG/Public/Combat/Skills/SkillTypes.h"
#include "JRPGSkillComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSkillCast, FName /*SkillId*/, AActor* /*Caster*/, int32 /*TargetCount*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnSkillResolvedDetailed, FName /*SkillId*/, AActor* /*Caster*/, int32/*TargetCount*/,bool/*bFromTacticalReservation*/);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UJRPGSkillComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UJRPGSkillComponent();

	UPROPERTY(EditAnywhere) 
	TArray<TObjectPtr<UJRPGSkillDataAsset>> KnownSkills;

	FOnSkillCast OnSkillCast;
	FOnSkillResolvedDetailed OnSkillResolvedDetailed;

	bool HasSkill(FName SkillId) const;
	void LearnSkill(UJRPGSkillDataAsset *Skill);

	float GetCooldownRemaining(FName SkillId) const;
	UJRPGSkillDataAsset* GetSkillDef(FName SkillId) const;

	// 기존 즉발 API 유지
	FSkillCastResult CastSkill(FName SkillId,const TArray<AActor*> &Targets,FName ReasonTag);

	// 연출/노티파이용 분리 API
	FSkillCastResult PrepareSkillCast(FName SkillId,const TArray<AActor*> &Targets,bool bFromTacticalReservation,FName ReasonTag);
	FSkillCastResult ResolvePreparedSkillCast();
	void CancelPreparedSkillCast(bool bRefundCost,FName ReasonTag);
	bool HasPreparedSkillCast() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
	USTRUCT()
	struct FPreparedSkillCast
		{
			GENERATED_BODY()

			UPROPERTY() TObjectPtr<UJRPGSkillDataAsset> Skill = nullptr;
			UPROPERTY() TArray<TWeakObjectPtr<AActor>> Targets;
			UPROPERTY() int32 CommittedAP = 0;
			UPROPERTY() int32 CommittedSP = 0;
			UPROPERTY() bool bFromTacticalReservation = false;
			UPROPERTY() FName ReasonTag = NAME_None;
		};

	UPROPERTY() TMap<FName,float> Cooldowns;
	UPROPERTY() bool bHasPrepared = false;
	UPROPERTY() FPreparedSkillCast Prepared;

	TWeakObjectPtr<class UCombatStatsComponent> Stats;
	TWeakObjectPtr<class UCombatHPComponent> HP;
	TWeakObjectPtr<class UCombatAPComponent> AP;
	TWeakObjectPtr<class USPComponent> SP;

	FSkillCastResult ValidateCast(const UJRPGSkillDataAsset &Skill,const TArray<AActor*> &Targets) const;
	void ApplySkillEffects(const UJRPGSkillDataAsset &Skill,const TArray<AActor*> &Targets,bool bFromTacticalReservation);
	bool IsHostileTarget(AActor *Target)const;
};