#include "Combat/Battle/BasicCombatSubsystem.h"

#include "Combat/Battle/CombatFormulaLibrary.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/Stats/CombatStatsComponent.h"

#include "Combat/Stats/CombatHPComponent.h"
#include "Combat/Stats/CombatAPComponent.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/SP/CombatSynergyPointSubsystem.h"

#include "Combat/Threat/CombatThreatComponent.h"
#include "Combat/Groggy/CombatGroggyComponent.h"

bool UBasicCombatSubsystem::IsFriendlyTarget(AActor* Attacker, AActor* Target) const
{
	if (!Attacker || !Target) return false;

	ICombatParticipantInterface* A = Cast<ICombatParticipantInterface>(Attacker);
	ICombatParticipantInterface* T = Cast<ICombatParticipantInterface>(Target);
	if (!A || !T) return false;

	const ECombatTeam TA = A->GetCombatTeam();
	const ECombatTeam TT = T->GetCombatTeam();

	if (TA == ECombatTeam::Neutral || TT == ECombatTeam::Neutral) return false;
	return TA == TT;
}

FCombatActionResult UBasicCombatSubsystem::ExecuteBasicAttack(const FBasicAttackRequest& Req)
{
	AActor* Attacker = Req.Attacker.Get();
	AActor* Target = Req.Target.Get();

	if (!Attacker || !Target) return FCombatActionResult::Fail("Reject.InvalidTarget");
	if (Attacker == Target) return FCombatActionResult::Fail("Reject.InvalidTarget");
	if (IsFriendlyTarget(Attacker, Target)) return FCombatActionResult::Fail("Reject.FriendlyTarget");

	UCombatHPComponent* AttackerHP = Attacker->FindComponentByClass<UCombatHPComponent>();
	UCombatAPComponent* AttackerAP = Attacker->FindComponentByClass<UCombatAPComponent>();
	USPComponent* AttackerSP = Attacker->FindComponentByClass<USPComponent>();
	UCombatStatsComponent* AttackerStats = Attacker->FindComponentByClass<UCombatStatsComponent>();

	UCombatHPComponent* TargetHP = Target->FindComponentByClass<UCombatHPComponent>();
	UCombatStatsComponent* TargetStats = Target->FindComponentByClass<UCombatStatsComponent>();
	UCombatThreatComponent* TargetThreat = Target->FindComponentByClass<UCombatThreatComponent>();
	UGroggyComponent* TargetGroggy = Target->FindComponentByClass<UGroggyComponent>();

	if (!AttackerHP || !TargetHP) return FCombatActionResult::Fail("Reject.MissingHP");
	if (AttackerHP->IsDead()) return FCombatActionResult::Fail("Reject.AttackerDead");
	if (TargetHP->IsDead()) return FCombatActionResult::Fail("Reject.TargetDead");

	if (Req.APCost > 0)
	{
		if (!AttackerAP || !AttackerAP->Consume(Req.APCost,Req.ReasonTag))
			return FCombatActionResult::Fail("Reject.NotEnoughAP");
	}

	const float Atk = AttackerStats ? AttackerStats->GetSnapshot().Attack : 10.f;
	const float Def = TargetStats ? TargetStats->GetSnapshot().Defense : 5.f;
	const float CritRate = AttackerStats ? AttackerStats->GetSnapshot().CritRate : 0.f;
	const float CritBonus = AttackerStats ? AttackerStats->GetSnapshot().CritDamage : 0.f;

	FCombatActionResult Out = FCombatActionResult::Ok();
	Out.Attacker = Attacker;
	Out.Target = Target;
	Out.Breakdown = UCombatFormulaLibrary::BuildDamage(
		Atk,
		Def,
		Req.BasePower,
		Req.AttackScale,
		Req.DefenseScale,
		Req.PowerMultiplier,
		Req.bAllowCrit,
		CritRate,
		CritBonus,
		Req.VarianceMin,
		Req.VarianceMax,
		Req.GroggyPower,
		Req.ThreatMultiplier
		);

	TargetHP->ApplyDamage(Out.Breakdown.FinalDamage, Attacker, Req.ReasonTag);
	
	if (TargetGroggy && Out.Breakdown.GroggyDamage > 0.f)
	{
		TargetGroggy->AddGroggyDamage(Out.Breakdown.GroggyDamage, Attacker, Req.ReasonTag);
	}
	
	if (UCombatSynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<UCombatSynergyPointSubsystem>() : nullptr)
	{
		if (Out.Breakdown.FinalDamage > 0.f)
		{
			SP->ReportDamage(Attacker,Target,Out.Breakdown.FinalDamage,false,Req.ReasonTag);
		}

		if (Out.Breakdown.GroggyDamage > 0.f)
		{
			SP->ReportBreak(Attacker, Target, Out.Breakdown.GroggyDamage, false, false, Req.ReasonTag);
		}
	}

	if (TargetThreat && Out.Breakdown.ThreatGenerated > 0.f)
	{
		TargetThreat->AddThreat(Attacker, Out.Breakdown.ThreatGenerated, Req.ReasonTag);
	}

	if (AttackerSP && Req.SPGainOnHit > 0)
	{
		AttackerSP->AddSP(Req.SPGainOnHit, Req.ReasonTag);
	}

	Out.bTargetDied = TargetHP->IsDead();
	if (Out.bTargetDied)
	{
		if (AttackerSP && Req.SPGainOnKill > 0)
		{
			AttackerSP->AddSP(Req.SPGainOnKill, "Combat.KillSP");
		}
		OnCombatantDefeated.Broadcast(Target, Attacker);
	}

	OnBasicAttackResolved.Broadcast(Out);
	return Out;
}