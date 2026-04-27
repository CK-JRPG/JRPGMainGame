#include "Combat/Battle/BasicCombatSubsystem.h"

#include "Combat/Battle/CombatFormulaLibrary.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/Stats/CharacterCombatStatsComponent.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/SP/SynergyPointSubsystem.h"
#include "Combat/Battle/BattleSessionSubsystem.h"

#include "Combat/Threat/ThreatComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/AI/CombatPartyAIComponent.h"

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

	UHPComponent* AttackerHP = Attacker->FindComponentByClass<UHPComponent>();
	UAPComponent* AttackerAP = Attacker->FindComponentByClass<UAPComponent>();
	USPComponent* AttackerSP = Attacker->FindComponentByClass<USPComponent>();
	UCharacterCombatStatsComponent* AttackerStats = Attacker->FindComponentByClass<UCharacterCombatStatsComponent>();

	UHPComponent* TargetHP = Target->FindComponentByClass<UHPComponent>();
	UCharacterCombatStatsComponent* TargetStats = Target->FindComponentByClass<UCharacterCombatStatsComponent>();
	UThreatComponent* TargetThreat = Target->FindComponentByClass<UThreatComponent>();
	UGroggyComponent* TargetGroggy = Target->FindComponentByClass<UGroggyComponent>();

	if (!AttackerHP || !TargetHP) return FCombatActionResult::Fail("Reject.MissingHP");
	if (AttackerHP->IsDead()) return FCombatActionResult::Fail("Reject.AttackerDead");
	if (TargetHP->IsDead()) return FCombatActionResult::Fail("Reject.TargetDead");

	if (Req.APCost > 0)
	{
		if (!AttackerAP || !AttackerAP->Consume(Req.APCost, Req.ReasonTag))
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

	if (UCombatPartyAIComponent* PartyAI = Target->FindComponentByClass<UCombatPartyAIComponent>())
	{
		PartyAI->NotifyDamagedBy(Attacker);
	}
	
	if (TargetGroggy && Out.Breakdown.GroggyDamage > 0.f)
	{
		TargetGroggy->AddGroggyDamage(Out.Breakdown.GroggyDamage, Attacker, Req.ReasonTag);
	}
	
	if (USynergyPointSubsystem* SP = GetWorld() ? GetWorld()->GetSubsystem<USynergyPointSubsystem>() : nullptr)
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
	}

	if (UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("BasicCombatSubsystem::ExecuteBasicAttack : before resolved broadcast | BattleActive=%s Phase=%d AttackerValid=%s TargetValid=%s TargetDied=%s Reason=%s"),
			Battle->IsBattleActive() ? TEXT("true") : TEXT("false"),
			(int32)Battle->GetPhase(),
			IsValid(Attacker) ? TEXT("true") : TEXT("false"),
			IsValid(Target) ? TEXT("true") : TEXT("false"),
			Out.bTargetDied ? TEXT("true") : TEXT("false"),
			*Req.ReasonTag.ToString());
	}

	OnBasicAttackResolved.Broadcast(Out);

	if (Out.bTargetDied)
	{
		if (UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("BasicCombatSubsystem::ExecuteBasicAttack : lethal hit before defeat broadcast | BattleActive=%s Phase=%d Attacker=%s Target=%s Reason=%s"),
				Battle->IsBattleActive() ? TEXT("true") : TEXT("false"),
				(int32)Battle->GetPhase(),
				*GetNameSafe(Attacker),
				*GetNameSafe(Target),
				*Req.ReasonTag.ToString());
		}

		OnCombatantDefeated.Broadcast(Target, Attacker);
	}

	return Out;
}
