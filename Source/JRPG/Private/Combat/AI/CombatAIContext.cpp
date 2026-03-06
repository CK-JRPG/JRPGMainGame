// Source/JRPGCombat/Private/Combat/AI/CombatAIContext.cpp
#include "Combat/AI/CombatAIContext.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"

// 우리 모듈 구조 헤더들(이미 만들어둔 것으로 가정)
#include "Combat/Infrastructure/BattleSessionSubsystem.h"
#include "Combat/Infrastructure/TacticalModeSubsystem.h"
#include "Combat/Infrastructure/TrinityChainSubsystem.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Status/StatusComponent.h"// (가정) 상태이상 시스템에서 제공
#include "Combat/Groggy/GroggyComponent.h"// (가정) 그로기 시스템에서 제공

UCombatAIContext* UCombatAIContext::Build(AActor *InOwner)
{
	if (!InOwner) return nullptr;

	UCombatAIContext*Ctx = NewObject<UCombatAIContext>(GetTransientPackage());
	Ctx->Owner = InOwner;

	Ctx->PullSubsystemsAndComponents();
	Ctx->PullDerivedFlags();
	Ctx->PullReservationSnapshot();
	return Ctx;
}

void UCombatAIContext::PullSubsystemsAndComponents()
{
	UWorld *World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	if (!World) return;

	Session  = World->GetSubsystem<UBattleSessionSubsystem>();
	Tactical = World->GetSubsystem<UTacticalModeSubsystem>();
	Chain    = World->GetSubsystem<UTrinityChainSubsystem>();

	HP     = Owner.IsValid() ? Owner->FindComponentByClass<UHPComponent>() : nullptr;
	AP     = Owner.IsValid() ? Owner->FindComponentByClass<UAPComponent>() : nullptr;
	Skill  = Owner.IsValid() ? Owner->FindComponentByClass<USkillComponent>() : nullptr;
	Status = Owner.IsValid() ? Owner->FindComponentByClass<UStatusComponent>() : nullptr;
	Groggy = Owner.IsValid() ? Owner->FindComponentByClass<UGroggyComponent>() : nullptr;

	Threat = Owner.IsValid() ? Owner->FindComponentByClass<UThreatComponent>() : nullptr;// Enemy only
}

void UCombatAIContext::PullDerivedFlags()
{
	UWorld *World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	if (!World) return;

	NowReal = World->GetRealTimeSeconds();

	bSessionActive = (Session != nullptr) ? Session->IsSessionActive() : false;

	// 입력 잠금: 전술/체인에서 걸릴 수 있음(기본 이동 문서와 동일 계약)
	bInputLocked = (Session != nullptr) ? Session->IsInputLocked() : false;

	bChainActive = (Chain != nullptr) ? Chain->IsChainActive() : false;

	// “체인 진행 중 적 공격 금지” 상태를 별도 플래그로 제공한다고 가정
	bEnemySuppressed = (Chain!=nullptr) ? Chain -> IsEnemySuppressed() : false;
}

void UCombatAIContext::PullReservationSnapshot()
{
	Reservation = {};

	if (!Owner.IsValid() || !Tactical) return;

	// TacticalModeSubsystem이 “예약 데이터 제공만” 하고,
	// SkillComponent가 소비한다는 계약에 맞춰: AI는 “보류/우선순위 조정”에만 사용
	FName ReservedSkill;
	const bool bHas = Tactical->GetReservationSkillId(Owner.Get(),ReservedSkill);
	Reservation.bHasReservation = bHas;
	Reservation.ReservedSkillId = bHas ? ReservedSkill : NAME_None;
}

bool UCombatAIContext::IsAlive() const
{
	return HP ? HP->IsAlive() : true;
}

float UCombatAIContext::GetHPPercent() const
{
	return HP ? HP->GetHPPercent() : 1.0f;
}

bool UCombatAIContext::IsCCBlocked() const
{
	return Status ? Status->IsActionBlockedByCC() : false;
}

bool UCombatAIContext::IsGroggyStunned() const
{
	return Groggy ? (Groggy->GetPhase() == EGroggyPhase::Stunned) : false;
}

bool UCombatAIContext::IsRising() const
{
	return Groggy ? (Groggy->GetPhase() == EGroggyPhase::Rising) : false;
}

AActor* UCombatAIContext::GetPrimaryTarget() const
{
	return Session ?Session->GetPrimaryTarget() : nullptr;
}

AActor* UCombatAIContext::GetThreatTarget() const
{
	return Threat ? Threat->GetCurrentTarget() : nullptr;
}

void UCombatAIContext::GetPartyMembers(TArray<AActor*> &Out) const
{
	if (!Session) return;
	Session->GetPartyMembers(Out);
}

void UCombatAIContext::GetEnemies(TArray<AActor*> &Out) const
{
	if (!Session) return;
	Session->GetEnemies(Out);
}