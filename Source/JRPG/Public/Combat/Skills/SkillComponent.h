#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPG/Public/Combat/Skills/SkillDataAsset.h"
#include "JRPG/Public/Combat/Skills/SkillTypes.h"
#include "SkillComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSkillCast, FName /*SkillId*/, AActor* /*Caster*/, int32 /*TargetCount*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnSkillResolvedDetailed, FName /*SkillId*/, AActor* /*Caster*/, int32/*TargetCount*/,bool/*bFromTacticalReservation*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSkillCooldownFinished, FName /*SkillId*/);

USTRUCT()
struct FPreparedSkillCast
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<USkillDataAsset> Skill = nullptr;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> Targets;
	UPROPERTY() int32 CommittedAP = 0;
	UPROPERTY() int32 CommittedSP = 0;
	UPROPERTY() bool bFromTacticalReservation = false;
	UPROPERTY() FName ReasonTag = NAME_None;
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	USkillComponent();

	UPROPERTY(EditAnywhere) 
	TArray<TObjectPtr<USkillDataAsset>> KnownSkills;

	FOnSkillCast OnSkillCast;
	FOnSkillResolvedDetailed OnSkillResolvedDetailed;
	FOnSkillTargetResolved OnSkillTargetResolved;
	FOnSkillCooldownFinished OnSkillCooldownFinished;

	bool HasSkill(FName SkillId) const;
	void LearnSkill(USkillDataAsset *Skill);

	float GetCooldownRemaining(FName SkillId) const;
	USkillDataAsset* GetSkillDef(FName SkillId) const;
	void GetOwnedSkillIds(TArray<FName>& OutSkillIds) const;
	bool CanUseSkill(FName SkillId) const;
	void RequestBasicAttack(AActor* Target);
	void RequestUseSkillByAI(FName SkillId, AActor* Target);
	
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

	UPROPERTY() TMap<FName,float> Cooldowns;
	UPROPERTY() bool bHasPrepared = false;
	UPROPERTY() FPreparedSkillCast Prepared;

	TWeakObjectPtr<class UCharacterCombatStatsComponent> Stats;
	TWeakObjectPtr<class UHPComponent> HP;
	TWeakObjectPtr<class UAPComponent> AP;
	TWeakObjectPtr<class USPComponent> SP;

	FSkillCastResult ValidateCast(const USkillDataAsset &Skill,const TArray<AActor*> &Targets) const;
	void ApplySkillEffects(const USkillDataAsset &Skill,const TArray<AActor*> &Targets,bool bFromTacticalReservation);
	bool IsHostileTarget(AActor *Target)const;
};
