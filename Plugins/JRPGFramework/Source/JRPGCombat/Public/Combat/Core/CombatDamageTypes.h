#pragma once

#include "CoreMinimal.h"
#include "JRPGCoreApiTypes.h"
#include "Combat/Movement/CombatMotionTypes.h"  // HitMove archetype 연동
#include "Combat/Core/CombatDamageTypes.generated.h"

// 전투에서 “피해/회복”은 스킬/아이템/환경이 모두 호출 가능.
// Skill 시스템은 나중에 여기 Spec을 만들어 HPComponent에 전달하면 된다.

UENUM()
enum class ECombatDamageKind : uint8
{
	Damage,
	Heal
};

UENUM()
enum class ECombatHitReaction : uint8
{
	None,
	Knockback,       // 수동 이동
	Knockup,         // Launch
	KnockdownSlide,  // 수동 이동 + 마찰 무시
	SlamToGround,    // Launch downward
	WallSlam         // 수동 이동 + StopOnBlock + WallSlam event
};

USTRUCT()
struct FCombatDamageSpec
{
	GENERATED_BODY()

	// 공통
	UPROPERTY() ECombatDamageKind Kind = ECombatDamageKind::Damage;
	UPROPERTY() float Amount = 0.f;
	UPROPERTY() FName SourceTag = NAME_None;        // "Skill.BasicAttack", "Env.Spike", ...
	UPROPERTY() TObjectPtr<AActor> Instigator = nullptr; // 공격자/치료자
	UPROPERTY() TObjectPtr<AActor> Causer = nullptr;     // 발사체/트랩 등
	UPROPERTY() FVector HitWorldNormal = FVector::ZeroVector; // 피해 방향(피격 리액션에 사용)
	UPROPERTY() FVector HitWorldPoint = FVector::ZeroVector;

	// 전투 이동(HitMove) 요청
	UPROPERTY() bool bRequestHitMotion = true;
	UPROPERTY() ECombatHitReaction HitReaction = ECombatHitReaction::Knockback;

	// HitMotion 파라미터(공통)
	UPROPERTY() float HitDistance = 350.f;   // Knockback/Slide/WallSlam
	UPROPERTY() float HitDuration = 0.18f;
	UPROPERTY() float LaunchMaxTime = 1.2f; // Knockup/Slam
	UPROPERTY() bool bEndLaunchWhenGrounded = true;

	// Knockup: 위로 힘, Slam: 아래로 힘
	UPROPERTY() float LaunchUpZ = 900.f;
	UPROPERTY() float SlamDownZ = -2200.f;

	// WallSlam: 벽 맞으면 End.WallSlam + 이벤트
	UPROPERTY() float WallSlamDistance = 650.f;
	UPROPERTY() float WallSlamDuration = 0.25f;

	// 그로기 연동
	UPROPERTY() float BreakAmount = 0.f;

	// 어그로 연동(피해 받은 “적”이 Instigator를 더 보게)
	UPROPERTY() float ThreatAmount = 0.f;

	// SP 연동(기본전투에서 즉시 적용 가능)
	UPROPERTY() int32 SPOnHit = 0;
	UPROPERTY() int32 SPOnKill = 0;

	// 상태이상 연동(옵션): skill 시스템에서 채워 넣으면 됨.
	UPROPERTY() bool bApplyStatus = false;
	UPROPERTY() FName StatusId = NAME_None;
	UPROPERTY() float StatusDuration = 0.f; // 0이면 StatusDB 기본값 사용

	// 정책
	UPROPERTY() bool bRequireCombatActive = false; // 필요 시 전투중이 아니면 거부
	
	UPROPERTY() bool bFromTacticalReservation = false;
};

USTRUCT()
struct FCombatDamageResult
{
	GENERATED_BODY()

	UPROPERTY() FJRPGOpResult Op = FJRPGOpResult::Ok();
	UPROPERTY() float AppliedAmount = 0.f;
	UPROPERTY() float OldValue = 0.f;
	UPROPERTY() float NewValue = 0.f;
	UPROPERTY() bool bKilled = false;

	// 부가 정보(디버그/텔레메트리)
	UPROPERTY() FName EndReasonTag = NAME_None;
};