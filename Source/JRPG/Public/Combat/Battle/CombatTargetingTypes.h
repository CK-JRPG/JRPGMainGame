// Source/JRPGCombat/Public/Combat/Battle/CombatTargetingTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Skills/JRPGSkillTypes.h"
#include "CombatTargetingTypes.generated.h"
 
USTRUCT()
struct FTargetingResult
{
	GENERATED_BODY()

	UPROPERTY() bool bOk = false;
	UPROPERTY() FName ReasonTag = NAME_None;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> Targets;

	static FTargetingResult Ok(const TArray<AActor*> &InTargets)
	{
		FTargetingResult R;
		R.bOk = true;
		for (AActor *A : InTargets)
		{
			if (A)R.Targets.Add(A);
		}
		return R;
	}

	static FTargetingResult Fail(FName Reason)
	{
		FTargetingResult R;
		R.bOk = false;
		R.ReasonTag = Reason;
		return R;
	}
};

USTRUCT()
struct FTargetValidationResult
{
	GENERATED_BODY()

	UPROPERTY() bool bOk = false;
	UPROPERTY() FName ReasonTag = NAME_None;

	static FTargetValidationResult Ok()
	{
		FTargetValidationResult R;
		R.bOk = true;
		return R;
	}

	static FTargetValidationResult Fail(FName Reason)
	{
		FTargetValidationResult R;
		R.bOk = false;
		R.ReasonTag = Reason;
		return R;
	}
};