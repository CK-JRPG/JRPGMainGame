#include"Combat/Infrastructure/BattleSessionSubsystem.h"

void UBattleSessionSubsystem::SetPhase(EJRPGCombatPhase NewPhase)
{
	if (Phase == NewPhase) return;
	const EJRPGCombatPhase Prev =Phase;
	Phase = NewPhase;
	OnCombatPhaseChanged.Broadcast(Prev,NewPhase);
}

void UBattleSessionSubsystem::RegisterParticipant(AActor*Actor,bool bIsPlayerParty)
{
	if (!Actor) return;
	
	Participants.Add(Actor);
	if (bIsPlayerParty)
	{
		PlayerParty.Add(Actor);
	}
}

bool UBattleSessionSubsystem::IsParticipant(AActor *Actor)const
{
	return Actor&&Participants.Contains(Actor);
}

void UBattleSessionSubsystem::SetPrimaryTarget(AActor*Target)
{
	PrimaryTarget = Target;
}

void UBattleSessionSubsystem::GetPartyActors(TArray<AActor*>&OutParty)const
{
	OutParty.Reset();
	for (const TWeakObjectPtr<AActor> &W : PlayerParty)
	{
		if (AActor*A = W.Get())
		{
			OutParty.Add(A);
		}
	}
}

void UBattleSessionSubsystem::PushPlayerInputLock(FName OwnerTag)
{
	if (OwnerTag.IsNone())return;
	PlayerInputLockOwners.Add(OwnerTag);
}

void UBattleSessionSubsystem::PopPlayerInputLock(FName OwnerTag)
{
	if (OwnerTag.IsNone())return;
	for (int32 i =PlayerInputLockOwners.Num() - 1; i>=0; --i)
	{
		if (PlayerInputLockOwners[i] == OwnerTag)
		{
			PlayerInputLockOwners.RemoveAt(i);
			break;
		}
	}
}

void UBattleSessionSubsystem::RecomputeSuppressionScope()
{
	// 기본: 가장 강한 스코프로 수렴
	// StopAndGate > StopOnly/GateOnly
	EEnemySuppressionScope Best = EEnemySuppressionScope::StopOnly;
	for (const FSuppOwner &O : EnemySuppressionOwners)
	{
		if (O.Scope== EEnemySuppressionScope::StopAndGate)
		{
			Best = EEnemySuppressionScope::StopAndGate;
			break;
		}
		// GateOnly가 있으면 StopOnly보다 강하다고 보긴 어렵지만,
		// 게이트는 피해/디버프 차단에 필수라서 StopOnly보다 우선
		if (O.Scope == EEnemySuppressionScope::GateOnly)
		{
			Best = EEnemySuppressionScope::GateOnly;
		}
	}
	CurrentSuppressionScope = Best;
}

void UBattleSessionSubsystem::PushEnemySuppression(FName OwnerTag,EEnemySuppressionScope Scope)
{
	if (OwnerTag.IsNone()) return;

	FSuppOwner O;
	O.OwnerTag = OwnerTag;
	O.Scope = Scope;
	EnemySuppressionOwners.Add(O);

	RecomputeSuppressionScope();
}

void UBattleSessionSubsystem::PopEnemySuppression(FName OwnerTag)
{
	if (OwnerTag.IsNone()) return;

	for (int32 i =EnemySuppressionOwners.Num()-1; i>=0; --i)
	{
		if (EnemySuppressionOwners[i].OwnerTag == OwnerTag)
		{
			EnemySuppressionOwners.RemoveAt(i);
			break;
		}
	}
	RecomputeSuppressionScope();
}

bool UBattleSessionSubsystem::ShouldGateEnemyToAlly(AActor *Instigator, AActor *Victim) const
{
	if (!IsEnemySuppressed()) return false;

	// GateOnly 또는 StopAndGate에서만 게이트 발동
	if (!(CurrentSuppressionScope == EEnemySuppressionScope::GateOnly || CurrentSuppressionScope == EEnemySuppressionScope::StopAndGate))
		return false;

	if (!Instigator||!Victim)return false;

	const bool bInstigatorEnemy = IsEnemyActor(Instigator);
	const bool bVictimPlayer = IsPlayerActor(Victim);

	return bInstigatorEnemy && bVictimPlayer;
}