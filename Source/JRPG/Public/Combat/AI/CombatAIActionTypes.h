// Source/JRPGCombat/Public/Combat/AI/CombatAIActionTypes.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CombatAIActionTypes.generated.h"

UENUM(BlueprintType)
enum class EPartyAIState : uint8
{
	Follow,
	Engage,
	ExecuteRole,
	Reposition,
	ExecuteReservation,
	Recover,

	// Chain이 Active면 일반 AI를 완전히 억제
	SuppressedByChain,
};

UENUM(BlueprintType)
enum class EEnemyCombatState : uint8
{
	Idle,
	Engage,
	Combat_Normal,
	Groggy_Stunned,
	Rising,
	Disengage,

	// Chain 시퀀스 동안 적 AI도 억제 (연출/입력 레이어 분리)
	SuppressedByChain,
};

UENUM(BlueprintType)
enum class ECombatAIActionType : uint8
{
	None,
	BasicAttack,
	UseSkill,
	Wait,
};

USTRUCT(BlueprintType)
struct FCombatAIAction
{
	GENERATED_BODY()

	UPROPERTY() ECombatAIActionType Type = ECombatAIActionType::None;
	UPROPERTY() FName SkillId = NAME_None;
	UPROPERTY() TWeakObjectPtr<AActor> Target = nullptr;
	UPROPERTY() float Score = -FLT_MAX;

	static FCombatAIAction MakeWait(float InScore = 0.f)
	{
		FCombatAIAction A;
		A.Type = ECombatAIActionType::Wait;
		A.Score = InScore;
		return A;
	}

	static FCombatAIAction MakeBasicAttack (TWeakObjectPtr<AActor> InTarget, float InScore)
	{
		FCombatAIAction A;
		A.Type = ECombatAIActionType::BasicAttack;
		A.Target = InTarget;
		A.Score = InScore;
		return A;
	}

	static FCombatAIAction MakeUseSkill(FName InSkillId, TWeakObjectPtr<AActor> InTarget, float InScore)
	{
		FCombatAIAction A;
		A.Type = ECombatAIActionType::UseSkill;
		A.SkillId = InSkillId;
		A.Target = InTarget;
		A.Score = InScore;
		return A;
	}
};