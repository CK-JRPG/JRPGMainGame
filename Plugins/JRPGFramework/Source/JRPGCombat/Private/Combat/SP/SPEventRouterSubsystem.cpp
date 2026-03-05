#include "Combat/SP/SPEventRouterSubsystem.h"

#include "Combat/Infrastructure/SynergyPointSubsystem.h"

USynergyPointSubsystem* USPEventRouterSubsystem::GetSPSubsystem() const
{
	if (UWorld* W = GetWorld())
	{
		return W->GetSubsystem<USynergyPointSubsystem>();
	}
	return nullptr;
}

void USPEventRouterSubsystem::RouteDamageEvent(const FCombatDamageSpec& Spec, const FCombatDamageResult& Result, AActor* Victim)
{
	USynergyPointSubsystem* SP = GetSPSubsystem();
	if (!SP) return;

	// 기본 정책(완전 동작):
	// - Spec.SPOnHit / SPOnKill 값이 지정되어 있으면 그만큼 증가
	// - SourceTag를 그대로 넣어 추적 가능하게 함
	if (Spec.Kind != ECombatDamageKind::Damage) return;
	if (!Result.Op.bOk) return;

	if (Spec.SPOnHit > 0)
	{
		FJRPGSPGainEvent Ev;
		Ev.Amount = Spec.SPOnHit;
		Ev.SourceTag = Spec.SourceTag.IsNone() ? FName("SP.OnHit") : Spec.SourceTag;
		Ev.Instigator = Spec.Instigator;
		SP->ApplyGainEvent(Ev);
	}

	if (Result.bKilled && Spec.SPOnKill > 0)
	{
		FJRPGSPGainEvent Ev;
		Ev.Amount = Spec.SPOnKill;
		Ev.SourceTag = Spec.SourceTag.IsNone() ? FName("SP.OnKill") : Spec.SourceTag;
		Ev.Instigator = Spec.Instigator;
		SP->ApplyGainEvent(Ev);
	}
}
