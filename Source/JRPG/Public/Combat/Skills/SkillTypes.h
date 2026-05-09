#pragma once
#include "CoreMinimal.h"
#include "SkillTypes.generated.h"

UENUM()
enum class ESkillTargetType : uint8
{
	Self,
	AllySingle,
	EnemySingle,
	AllyAll,
	EnemyAll
};

USTRUCT()
struct FSkillCastResult
{
	GENERATED_BODY()
	
	UPROPERTY() bool bOk = false;
	UPROPERTY() FName ReasonTag = NAME_None;

	static FSkillCastResult Ok()
	{
		FSkillCastResult R; 
		R.bOk = true; 
		
		return R;
	}
	
	static FSkillCastResult Fail (FName Reason)
	{
		FSkillCastResult R; 
		R.bOk = false; 
		R.ReasonTag = Reason; 
		
		return R;
	}
};

USTRUCT()
struct FSkillTargetResolvedEvent
{
	GENERATED_BODY()

	UPROPERTY() FName SkillId = NAME_None;
	UPROPERTY() TWeakObjectPtr<AActor> Caster;
	UPROPERTY() TWeakObjectPtr<AActor> Target;

	UPROPERTY() float FinalDamage = 0.f;
	UPROPERTY() float HealAmount = 0.f;
	UPROPERTY() bool bCritical = false;
	UPROPERTY() bool bStatusApplied = false;
	UPROPERTY() int32 StatusRemovedCount = 0;
	UPROPERTY() bool bFromTacticalReservation = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSkillTargetResolved, const FSkillTargetResolvedEvent& /*Event*/);
