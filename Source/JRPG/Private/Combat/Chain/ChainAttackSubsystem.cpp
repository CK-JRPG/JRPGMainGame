#include "Combat/Chain/ChainAttackSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Characters/CombatCharacterComponent.h"

#include "Combat/Stats/HPComponent.h"

UBattleSessionSubsystem* UChainAttackSubsystem::GetBattle() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

UBasicCombatSubsystem* UChainAttackSubsystem::GetBasicCombat() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBasicCombatSubsystem>() : nullptr;
}

bool UChainAttackSubsystem::IsPlayerActor(AActor* Actor) const
{
	if (!Actor) return false;
	if (ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor))
		return P->GetCombatTeam() == ECombatTeam::Player;
	return false;
}

bool UChainAttackSubsystem::IsValidMember(AActor* Actor) const
{
	if (!Actor) return false;
	if (!IsPlayerActor(Actor)) return false;

	if (ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor))
	{
		if (UHPComponent* HP = P->GetHP())
			return !HP->IsDead();
	}
	return false;
}

bool UChainAttackSubsystem::BuildMemberList(const FChainAttackConfig& Config, AActor* Starter)
{
	ActiveMembers.Reset();

	TArray<AActor*> Members;

	if (Config.ChainMembers.Num() > 0)
	{
		for (const TWeakObjectPtr<AActor>& W : Config.ChainMembers)
		{
			if (AActor* A = W.Get())
				Members.Add(A);
		}
	}
	else
	{
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (UPartySubsystem* Party = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
			{
				Party->GetPartyMembers(Members);
			}
		}
	}

	TSet<AActor*> Dedup;
	for (AActor* A : Members)
	{
		if (!A || Dedup.Contains(A)) continue;
		if (!IsValidMember(A)) continue;

		ActiveMembers.Add(A);
		Dedup.Add(A);
	}

	if (Starter && !Dedup.Contains(Starter) && IsValidMember(Starter))
	{
		ActiveMembers.Insert(Starter,0);
	}

	return ActiveMembers.Num() > 0;
}

bool UChainAttackSubsystem::TryStartChain(AActor* Starter, const FChainAttackConfig& Config)
{
	if (IsActive()) return false;
	if (!Starter) return false;
	if (!IsPlayerActor(Starter)) return false;

	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Battle || !Battle->IsBattleActive()) return false;
	if (!Battle->CanActorActNow(Starter)) return false;

	if (!BuildMemberList(Config, Starter)) return false;

	if (!Battle->PauseFlow("ChainAttack"))
		return false;

	ActiveConfig = Config;

	Snapshot = FChainAttackSnapshot();
	Snapshot.BattleSessionId = Battle->GetSnapshot().SessionId;
	Snapshot.ChainId = FGuid::NewGuid();
	Snapshot.State = EChainAttackState::Active;
	Snapshot.RemainingChainPoints = FMath::Max(1, Config.StartingChainPoints);
	Snapshot.StepIndex = 0;
	Snapshot.CurrentDamageMultiplier = FMath::Max(0.1f, Config.BaseDamageMultiplier);
	Snapshot.ChainStarter = Starter;
	Snapshot.CurrentActor = ActiveMembers[0];

	OnChainAttackStarted.Broadcast(Snapshot);
	return true;
}

void UChainAttackSubsystem::AdvanceChainActor()
{
	if (ActiveMembers.Num() <= 0)
	{
		Snapshot.CurrentActor = nullptr;
		return;
	}

	const int32 Num = ActiveMembers.Num();
	for (int32 Offset = 1; Offset <= Num; ++Offset)
	{
		const int32 NextIndex = (Snapshot.StepIndex + Offset) % Num;
		if (AActor* Next = ActiveMembers[NextIndex].Get())
		{
			if (IsValidMember(Next))
			{
				Snapshot.StepIndex = NextIndex;
				Snapshot.CurrentActor = Next;
				return;
			}
		}
	}

	Snapshot.CurrentActor = nullptr;
}

FCombatActionResult UChainAttackSubsystem::ExecuteChainBasicAttack(AActor* User,AActor* Target)
{
	if (!IsActive()) return FCombatActionResult::Fail("Reject.NoChainAttack");
	if (!User || !Target) return FCombatActionResult::Fail("Reject.InvalidTarget");
	if (Snapshot.CurrentActor.Get() != User) return FCombatActionResult::Fail("Reject.NotCurrentChainActor");
	if (!IsValidMember(User)) return FCombatActionResult::Fail("Reject.InvalidChainMember");

	UBasicCombatSubsystem* Basic = GetBasicCombat();
	if (!Basic) return FCombatActionResult::Fail("Reject.NoBasicCombat");

	UCombatCharacterComponent* CharComp = User->FindComponentByClass<UCombatCharacterComponent>();
	UCombatCharacterDataAsset* Def = CharComp ? CharComp->CharacterDef : nullptr;

	FBasicAttackRequest Req;
	Req.Attacker = User;
	Req.Target = Target;

	Req.BasePower = Def ? Def->BasicAttackBasePower : 5.f;
	Req.AttackScale = Def ? Def->BasicAttackAttackScale : 1.f;
	Req.DefenseScale = Def ? Def->BasicAttackDefenseScale : 0.5f;

	Req.APCost = 0;
	Req.SPGainOnHit = 0;
	Req.SPGainOnKill = 0;

	Req.PowerMultiplier = Snapshot.CurrentDamageMultiplier;
	Req.GroggyPower = (Def ? Def->BasicAttackGroggyPower : 8.f) + ActiveConfig.GroggyPowerBonus;
	Req.ThreatMultiplier = 0.f;
	Req.ReasonTag = "Chain.BasicAttack";

	FCombatActionResult R = Basic->ExecuteBasicAttack(Req);
	if (!R.bOk) return R;

	Snapshot.RemainingChainPoints--;
	Snapshot.CurrentDamageMultiplier += ActiveConfig.BonusDamagePerStep;

	OnChainAttackStepResolved.Broadcast(Snapshot, User);

	if (Snapshot.RemainingChainPoints <= 0)
	{
		EndChain("Chain.PointsExhausted");
		return R;
	}

	AdvanceChainActor();

	if (!Snapshot.CurrentActor.IsValid())
	{
		EndChain("Chain.NoValidMember");
	}

	return R;
}

void UChainAttackSubsystem::EndChain(FName)
{
	if (!IsActive()) return;

	FChainAttackSnapshot Final = Snapshot;
	Final.State = EChainAttackState::Finishing;

	if (UBattleSessionSubsystem* Battle = GetBattle())
	{
		Battle->ResumeFlow("ChainAttack");

		if (ActiveConfig.bConsumeBattleTurnOnEnd && Battle->IsBattleActive())
		{
			Battle->FinishCurrentTurn("Chain.End");
		}
	}

	Snapshot = FChainAttackSnapshot();
	ActiveMembers.Reset();
	ActiveConfig = FChainAttackConfig();

	OnChainAttackEnded.Broadcast(Final);
}