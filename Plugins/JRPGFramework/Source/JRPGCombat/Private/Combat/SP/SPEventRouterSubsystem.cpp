#include "Combat/SP/SPEventRouterSubsystem.h"

#include "Combat/Infrastructure/SynergyPointSubsystem.h"
#include "Combat/Stats/HPComponent.h"

void USPEventRouterSubsystem::RouteDamageOrHeal(const FCombatDamageSpec& Spec, const FCombatDamageResult& Result, AActor* Victim)
{
	if (!Result.Op.bOk) return;
	if (!Spec.Instigator) return;

	USynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<USynergyPointSubsystem>() : nullptr;
	if (!SP) return;

	// Damage / Heal 이벤트
	FSPGainEvent Ev;
	Ev.Instigator = Spec.Instigator;
	Ev.Target = Victim;
	Ev.SourceTag = Spec.SourceTag;
	Ev.bFromTacticalReservation = Spec.bFromTacticalReservation;
	Ev.OutcomeValue = (float)Result.AppliedAmount;

	if (Spec.Kind == ECombatDamageKind::Damage)
	{
		Ev.Type = ESPEventType::Damage;
		SP->SubmitGainEvent(Ev, "SP.Base.Damage");

		// Break 기여가 있으면 별도 이벤트(문서 5.1 + 5.2.2)
		if (Spec.BreakAmount > 0.f)
		{
			FSPGainEvent BreakEv = Ev;
			BreakEv.Type = ESPEventType::Break;
			BreakEv.OutcomeValue = Spec.BreakAmount;
			SP->SubmitGainEvent(BreakEv, "SP.Base.Break");
		}

		// Taunt/Threat 변화(문서 5.1 “도발/위협 성공”)
		if (Spec.ThreatAmount > 0.f)
		{
			FSPGainEvent TauntEv = Ev;
			TauntEv.Type = ESPEventType::Taunt;
			TauntEv.OutcomeValue = Spec.ThreatAmount;
			SP->SubmitGainEvent(TauntEv, "SP.Base.Taunt");
		}
	}
	else
	{
		// Heal: CriticalHeal 판정을 위해 힐 직전 HP 비율 전달
		Ev.Type = ESPEventType::Heal;

		if (Victim)
		{
			if (UHPComponent* HP = Victim->FindComponentByClass<UHPComponent>())
			{
				const float MaxHP = FMath::Max(1.f, HP->MaxHP);
				Ev.TargetHPBeforeRatio = (float)(Result.OldValue / MaxHP);
			}
		}

		SP->SubmitGainEvent(Ev, "SP.Base.Heal");
	}
}

void USPEventRouterSubsystem::RouteStatusApplied(AActor* Instigator, AActor* Target, FName StatusId, bool bDebuff, bool bFromTacticalReservation, FName SourceTag)
{
	if (!Instigator) return;

	USynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<USynergyPointSubsystem>() : nullptr;
	if (!SP) return;

	FSPGainEvent Ev;
	Ev.Instigator = Instigator;
	Ev.Target = Target;
	Ev.SourceTag = SourceTag;
	Ev.bFromTacticalReservation = bFromTacticalReservation;
	Ev.OutcomeValue = 1.f;

	Ev.Type = bDebuff ? ESPEventType::Debuff : ESPEventType::Buff;
	SP->SubmitGainEvent(Ev, bDebuff ? "SP.Base.Debuff" : "SP.Base.Buff");
}

void USPEventRouterSubsystem::RouteStatusCleansed(AActor* Instigator, AActor* Target, FName RemovedStatusId, bool bFromTacticalReservation, FName SourceTag)
{
	if (!Instigator) return;

	USynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<USynergyPointSubsystem>() : nullptr;
	if (!SP) return;

	FSPGainEvent Ev;
	Ev.Instigator = Instigator;
	Ev.Target = Target;
	Ev.SourceTag = SourceTag;
	Ev.bFromTacticalReservation = bFromTacticalReservation;
	Ev.OutcomeValue = 1.f;

	Ev.Type = ESPEventType::Cleanse;
	SP->SubmitGainEvent(Ev, "SP.Role.Cleanse");
}

void USPEventRouterSubsystem::RouteSupporterBuffUptime(AActor* Supporter, AActor* Target, float UptimeSec, bool bFromTacticalReservation, FName SourceTag)
{
	if (!Supporter) return;

	USynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<USynergyPointSubsystem>() : nullptr;
	if (!SP) return;

	FSPGainEvent Ev;
	Ev.Instigator = Supporter;
	Ev.Target = Target;
	Ev.SourceTag = SourceTag;
	Ev.bFromTacticalReservation = bFromTacticalReservation;
	Ev.OutcomeValue = UptimeSec;

	Ev.Type = ESPEventType::SupporterBuffUptime;
	SP->SubmitGainEvent(Ev, "SP.Role.BuffUptime");
}