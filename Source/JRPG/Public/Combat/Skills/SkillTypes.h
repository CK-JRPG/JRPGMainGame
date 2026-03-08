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