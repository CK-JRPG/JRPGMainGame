// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "Combat/Core/CombatDamageTypes.h"
#include "Combat/Movement/JRPGCombatMotionTypes.h"
#include "JRPGSkillTypes.generated.h"

/** 스킬 타겟팅 타입(기본전투/스킬 공통) */
UENUM()
enum class EJRPGSkillTargeting : uint8
{
	Self,
	SingleActor,
	MultiActor,
	Point,
	SelfAndTarget
};

/** 스킬 이펙트 종류(확장 안전) */
UENUM()
enum class EJRPGSkillEffectKind : uint8
{
	DealDamage,       // HPComponent.ApplyDamageSpec 사용
	Heal,             // HPComponent.ApplyDamageSpec 사용(Kind=Heal)
	ApplyStatus,      // StatusComponent.ApplyStatus
	RemoveStatus,     // StatusComponent.RemoveStatus
	AddGroggy,        // GroggyComponent.AddBreak
	AddThreat,        // ThreatComponent.AddThreat (직접)
	RequestMotion,    // CombatMotionComponent.RequestCombatMotion (SkillMove)
	GrantSP           // SPEventRouter로 라우팅 또는 직접 SPSubsystem(선택)
};

/** 이펙트 적용 대상 선택 */
UENUM()
enum class EJRPGEffectTarget : uint8
{
	Self,
	PrimaryTarget,
	AllTargets
};

UENUM()
enum class EJRPGExecutionBudget : uint8
{
	Normal,
	ChainFree
};

/** 스킬 비용/쿨 */
USTRUCT()
struct FJRPGSkillCost
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) int32 APCost = 0;
};

USTRUCT()
struct FJRPGSkillCooldown
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) float CooldownSec = 0.f;        // 개별 쿨
	UPROPERTY(EditDefaultsOnly) float GlobalCooldownSec = 0.f;  // GCD(선택)
};

/** 데미지/회복 이펙트 파라미터 */
USTRUCT()
struct FJRPGSkillDamageEffect
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly) float Amount = 10.f;
	UPROPERTY(EditDefaultsOnly) ECombatHitReaction HitReaction = ECombatHitReaction::None;

	// 기본전투 연동
	UPROPERTY(EditDefaultsOnly) float BreakAmount = 0.f;
	UPROPERTY(EditDefaultsOnly) float ThreatAmount = 0.f;
	UPROPERTY(EditDefaultsOnly) int32 SPOnHit = 0;
	UPROPERTY(EditDefaultsOnly) int32 SPOnKill = 0;
	
	// HitMotion 파라미터(기본전투 Spec에 들어가서 HPComponent가 처리)
	UPROPERTY(EditDefaultsOnly) float HitDistance = 350.f;
	UPROPERTY(EditDefaultsOnly) float HitDuration = 0.18f;
	UPROPERTY(EditDefaultsOnly) float LaunchUpZ = 900.f;
	UPROPERTY(EditDefaultsOnly) float SlamDownZ = -2200.f;
	UPROPERTY(EditDefaultsOnly) float LaunchMaxTime = 1.2f;

	UPROPERTY(EditDefaultsOnly) float WallSlamDistance = 650.f;
	UPROPERTY(EditDefaultsOnly) float WallSlamDuration = 0.25f;
};

/** 상태이상 이펙트 파라미터 */
USTRUCT()
struct FJRPGSkillStatusEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) FName StatusId = NAME_None;
	UPROPERTY(EditDefaultsOnly) float Duration = 0.f; // 0이면 StatusDB 기본값
};


/** 그로기 이펙트 파라미터 */
USTRUCT()
struct FJRPGSkillGroggyEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) float BreakAmount = 10.f;
};

/** 어그로 이펙트 파라미터(직접 가산) */
USTRUCT()
struct FJRPGSkillThreatEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) float ThreatAmount = 10.f;
};


/** 전투 이동(스킬 이동) 이펙트 파라미터 */
UENUM()
enum class EJRPGSkillMotionDirection : uint8
{
	Forward,        // 캐릭터 forward
	TowardTarget,   // 타겟 방향
	AwayFromTarget, // 타겟 반대
	CustomWorld     // Request.Direction 사용
};

USTRUCT()
struct FJRPGSkillMotionEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) ECombatMotionArchetype Archetype = ECombatMotionArchetype::Dash;
	UPROPERTY(EditDefaultsOnly) ECombatMotionExecMode ExecMode = ECombatMotionExecMode::DistanceReached;

	UPROPERTY(EditDefaultsOnly) EJRPGSkillMotionDirection DirectionMode = EJRPGSkillMotionDirection::Forward;
	UPROPERTY(EditDefaultsOnly) FVector CustomWorldDirection = FVector::ForwardVector;

	UPROPERTY(EditDefaultsOnly) float Distance = 450.f;
	UPROPERTY(EditDefaultsOnly) float Duration = 0.20f;

	UPROPERTY(EditDefaultsOnly) bool bIgnoreFriction = true;
	UPROPERTY(EditDefaultsOnly) ECombatMotionEndPolicy EndPolicy = ECombatMotionEndPolicy::StopOnBlock;

	UPROPERTY(EditDefaultsOnly) bool bFaceMoveDirection = true;
	UPROPERTY(EditDefaultsOnly) float FaceYawInterpSpeed = 25.f;
};

/** SP 이펙트 파라미터(간단) */
USTRUCT()
struct FJRPGSkillSPEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) int32 Amount = 0;
	UPROPERTY(EditDefaultsOnly) FName SourceTag = NAME_None;
};


/** 이펙트 엔트리(하나의 스킬은 여러 엔트리 가능) */
USTRUCT()
struct FJRPGSkillEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) EJRPGSkillEffectKind Kind = EJRPGSkillEffectKind::DealDamage;
	UPROPERTY(EditDefaultsOnly) EJRPGEffectTarget Target = EJRPGEffectTarget::PrimaryTarget;

	// Variant payloads (Kind에 따라 사용)
	UPROPERTY(EditDefaultsOnly) FJRPGSkillDamageEffect Damage;
	UPROPERTY(EditDefaultsOnly) FJRPGSkillStatusEffect Status;
	UPROPERTY(EditDefaultsOnly) FJRPGSkillGroggyEffect Groggy;
	UPROPERTY(EditDefaultsOnly) FJRPGSkillThreatEffect Threat;
	UPROPERTY(EditDefaultsOnly) FJRPGSkillMotionEffect Motion;
	UPROPERTY(EditDefaultsOnly) FJRPGSkillSPEffect SP;
};


/** 스킬 사용 요청 */
USTRUCT()
struct FJRPGSkillRequest
{
	GENERATED_BODY()

	UPROPERTY() FName SkillId = NAME_None;

	UPROPERTY() TObjectPtr<AActor> Instigator = nullptr; // 없으면 Owner 사용
	UPROPERTY() TObjectPtr<AActor> PrimaryTarget = nullptr;
	UPROPERTY() TArray<TObjectPtr<AActor>> AdditionalTargets;

	UPROPERTY() FVector TargetLocation = FVector::ZeroVector;

	// Tactical/Chain 등에서 사용할 “프리 실행” 옵션(완전성)
	UPROPERTY() bool bIgnoreCost = false;
	UPROPERTY() bool bIgnoreCooldown = false;
	UPROPERTY() bool bIgnoreGlobalCooldown = false;

	UPROPERTY() FName SourceTag = NAME_None; // "Tactical", "Chain", "AI" 등
	UPROPERTY() EJRPGExecutionBudget ExecutionBudget = EJRPGExecutionBudget::Normal;
	
	// 체인에서 “eligible만 허용” 정책을 강제하고 싶을 때
	UPROPERTY() bool bRequireChainEligible = false;
	
	// 체인 TP/피니셔 스케일링을 위해, 피해량 스칼라(기본 1)
	UPROPERTY() float DamageScalar = 1.0f;
	
	UPROPERTY() bool bFromTacticalReservation = false;

};

/** 스킬 실행 결과 */
USTRUCT()
struct FJRPGSkillResult
{
	GENERATED_BODY()

	UPROPERTY() FJRPGOpResult Op = FJRPGOpResult::Ok();

	UPROPERTY() FName SkillId = NAME_None;
	UPROPERTY() bool bExecuted = false;

	UPROPERTY() float StartedCooldown = 0.f;
	UPROPERTY() float StartedGlobalCooldown = 0.f;

	UPROPERTY() int32 SpentAP = 0;

	// 디버그/텔레메트리
	UPROPERTY() TArray<FName> AppliedEffectTags;
};


