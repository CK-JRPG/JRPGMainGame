#include "Combat/Characters/CharacterRuntimeSubsystem.h"

bool UCharacterRuntimeSubsystem::CommitState(const FName& CharacterID, const FCharacterRuntimeState& State)
{
	if (CharacterID.IsNone() || !State.IsValid())
	{
		return false;
	}

	RuntimeStateMap.Add(CharacterID, State);

	UE_LOG(LogTemp, Log,
		TEXT("CharacterRuntimeSubsystem : 런타임 상태 저장 [%s] HP=%.1f/%.1f AP=%d/%d SP=%d/%d"),
		*CharacterID.ToString(), State.HP, State.MaxHP, State.AP, State.MaxAP, State.SP, State.MaxSP);
	return true;
}

bool UCharacterRuntimeSubsystem::CommitStates(const TMap<FName, FCharacterRuntimeState>& States)
{
	// Validate the whole batch first so a malformed state cannot cause a partial commit.
	for (const TPair<FName, FCharacterRuntimeState>& Pair : States)
	{
		if (Pair.Key.IsNone() || !Pair.Value.IsValid())
		{
			return false;
		}
	}

	for (const TPair<FName, FCharacterRuntimeState>& Pair : States)
	{
		RuntimeStateMap.Add(Pair.Key, Pair.Value);
	}
	return true;
}

void UCharacterRuntimeSubsystem::InitializeSnapshotIfAbsent(const FName& CharacterID, float MaxHP, int32 MaxAP,
	int32 MaxSP)
{
	if (CharacterID.IsNone() || RuntimeStateMap.Contains(CharacterID))
	{
		return;
	}

	FCharacterRuntimeState State;
	State.HP = MaxHP;
	State.MaxHP = MaxHP;
	State.AP = MaxAP;
	State.MaxAP = MaxAP;
	State.SP = 0;
	State.MaxSP = MaxSP;
	State.Normalize();

	if (!CommitState(CharacterID, State))
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterRuntimeSubsystem : 초기 런타임 상태 생성 실패 [%s]"), *CharacterID.ToString());
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("CharacterRuntimeSubsystem : 초기 런타임 상태 생성 [%s] HP=%.1f AP=%d SP=%d"),
		*CharacterID.ToString(), State.MaxHP, State.MaxAP, State.SP);
}

void UCharacterRuntimeSubsystem::RecoverPartyFromWipe(const TArray<FName>& ActivePartyIds, float HPRecoverRatio, float APRecoverRatio)
{
	if (RuntimeStateMap.IsEmpty() || ActivePartyIds.IsEmpty())
	{
		return;
	}

	bool bHasAliveMember = false;
	bool bHasValidState = false;
	for (const FName& CharacterID : ActivePartyIds)
	{
		const FCharacterRuntimeState* State = RuntimeStateMap.Find(CharacterID);
		if (!State || !State->IsValid())
		{
			continue;
		}

		bHasValidState = true;
		if (State->HP > 0.f)
		{
			bHasAliveMember = true;
			break;
		}
	}

	if (!bHasValidState || bHasAliveMember)
	{
		return;
	}

	const float ClampedHPRatio = FMath::Clamp(HPRecoverRatio, 0.f, 1.f);
	const float ClampedAPRatio = FMath::Clamp(APRecoverRatio, 0.f, 1.f);

	int32 RecoveredCount = 0;
	for (const FName& CharacterID : ActivePartyIds)
	{
		FCharacterRuntimeState* State = RuntimeStateMap.Find(CharacterID);
		if (!State || !State->IsValid())
		{
			continue;
		}

		const float RecoveredHP = FMath::Max(1.f, State->MaxHP * ClampedHPRatio);
		State->HP = FMath::Clamp(RecoveredHP, 1.f, State->MaxHP);
		State->AP = FMath::Clamp(FMath::RoundToInt(static_cast<float>(State->MaxAP) * ClampedAPRatio), 0, State->MaxAP);

		OnHPChanged.Broadcast(CharacterID, State->HP, State->MaxHP);
		OnAPChanged.Broadcast(CharacterID, State->AP, State->MaxAP);
		++RecoveredCount;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("CharacterRuntimeSubsystem : 현재 파티 전멸 감지. 필드 복귀용 리커버리 적용 Count=%d (HP %.0f%% / AP %.0f%%)"),
		RecoveredCount, ClampedHPRatio * 100.f, ClampedAPRatio * 100.f);
}

bool UCharacterRuntimeSubsystem::RecoverPartyAfterVictory(const TArray<FName>& ActivePartyIds, float HPRecoverRatio)
{
	if (RuntimeStateMap.IsEmpty() || ActivePartyIds.IsEmpty())
	{
		return false;
	}

	const float ClampedRatio = FMath::Clamp(HPRecoverRatio, 0.f, 1.f);
	bool bStillRecovering = false;

	for (const FName& CharacterID : ActivePartyIds)
	{
		FCharacterRuntimeState* State = RuntimeStateMap.Find(CharacterID);
		if (!State || !State->IsValid())
		{
			continue;
		}

		if (State->HP < State->MaxHP)
		{
			const float RecoveryAmount = State->MaxHP * ClampedRatio;
			State->HP = FMath::Min(State->HP + RecoveryAmount, State->MaxHP);
			OnHPChanged.Broadcast(CharacterID, State->HP, State->MaxHP);

			if (State->HP < State->MaxHP)
			{
				bStillRecovering = true;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CharacterRuntimeSubsystem : 승리 후 HP 회복 (비율 %.0f%%, 잔여 회복 필요: %s)"),
		ClampedRatio * 100.f, bStillRecovering ? TEXT("Yes") : TEXT("No"));
	return bStillRecovering;
}

bool UCharacterRuntimeSubsystem::HasSnapshot(const FName& CharacterID) const
{
	const FCharacterRuntimeState* State = RuntimeStateMap.Find(CharacterID);
	return State && State->IsValid();
}

void UCharacterRuntimeSubsystem::ModifyHP(const FName& CharacterID, float Delta)
{
	FCharacterRuntimeState* State = RuntimeStateMap.Find(CharacterID);
	if (!State)
	{
		return;
	}

	const float OldHP = State->HP;
	State->HP = FMath::Clamp(State->HP + Delta, 0.f, State->MaxHP);

	UE_LOG(LogTemp, Log, TEXT("CharacterRuntimeSubsystem : HP 수정 [%s] %.1f → %.1f"),
		*CharacterID.ToString(), OldHP, State->HP);
	OnHPChanged.Broadcast(CharacterID, State->HP, State->MaxHP);
}

void UCharacterRuntimeSubsystem::ModifyAP(const FName& CharacterID, int32 Delta)
{
	FCharacterRuntimeState* State = RuntimeStateMap.Find(CharacterID);
	if (!State)
	{
		return;
	}

	State->AP = FMath::Clamp(State->AP + Delta, 0, State->MaxAP);
	OnAPChanged.Broadcast(CharacterID, State->AP, State->MaxAP);
}

void UCharacterRuntimeSubsystem::ModifySP(const FName& CharacterID, int32 Delta)
{
	FCharacterRuntimeState* State = RuntimeStateMap.Find(CharacterID);
	if (!State)
	{
		return;
	}

	State->SP = FMath::Clamp(State->SP + Delta, 0, State->MaxSP);
	OnSPChanged.Broadcast(CharacterID, State->SP, State->MaxSP);
}

const FCharacterRuntimeState* UCharacterRuntimeSubsystem::GetState(const FName& CharacterID) const
{
	return RuntimeStateMap.Find(CharacterID);
}
