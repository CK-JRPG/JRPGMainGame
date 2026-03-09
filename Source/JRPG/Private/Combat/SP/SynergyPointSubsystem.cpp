// Source/JRPGCombat/Private/Combat/SP/SynergyPointSubsystem.cpp
#include "Combat/SP/SynergyPointSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Chain/ChainAttackSubsystem.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Characters/CombatParticipantInterface.h"

void USynergyPointSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	State = FJRPGSynergyPointState();
	State.SPCap = Tuning.SPCap;
	TryBindExternalEvents();
}

void USynergyPointSubsystem::TryBindExternalEvents()
{
	if (bSubscribed || !GetWorld())
		return;
	

	if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		Battle->OnBattleEnded.AddUObject(this, &USynergyPointSubsystem::HandleBattleEnded);
	}

	if (UChainAttackSubsystem* Chain = GetWorld()->GetSubsystem<UChainAttackSubsystem>())
	{
		Chain->OnChainAttackEnded.AddUObject(this, &USynergyPointSubsystem::HandleChainEnded);
	}

	bSubscribed = true;
}

bool USynergyPointSubsystem::CanAcceptGain() const
{
	if (!GetWorld())
		return false;

	const UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	return Battle && Battle->IsBattleActive();
}

EJRPGPartyRole USynergyPointSubsystem::ResolveRoleForActor(AActor* Actor) const
{
	if (!Actor)
		return EJRPGPartyRole::Attacker;

	if (UCombatCharacterComponent* CC =Actor->FindComponentByClass<UCombatCharacterComponent>())
	{
		if (CC->CharacterDef)
		{
			return CC->CharacterDef->DefaultRole;
		}
	}

	return EJRPGPartyRole::Attacker;
}

bool USynergyPointSubsystem::IsAlly(AActor* A,AActor* B) const
{
	if (!A||!B)
		return false;

	ICombatParticipantInterface* PA =Cast<ICombatParticipantInterface>(A);
	ICombatParticipantInterface* PB =Cast<ICombatParticipantInterface>(B);
	
	if (!PA || !PB)
		return false;

	const ECombatTeam TA = PA->GetCombatTeam();
	const ECombatTeam TB = PB->GetCombatTeam();
	
	if (TA == ECombatTeam::Neutral || TB == ECombatTeam::Neutral)
		return false;

	return TA == TB;
}

bool USynergyPointSubsystem::IsEnemy(AActor* A,AActor* B) const
{
	if (!A || !B)
		return false;
	
	return !IsAlly(A, B);
}

int32 USynergyPointSubsystem::ComputeTacticalBonus(int32 RoleBonus,bool bFromTacticalReservation) const
{
	if (!bFromTacticalReservation || RoleBonus <= 0)
		return 0;
	

	const int32 Scaled = FMath::CeilToInt((float)RoleBonus * Tuning.TacticalRoleBonusMultiplier);
	return FMath::Max(0, Scaled - RoleBonus) + Tuning.TacticalFlatBonus;
}

bool USynergyPointSubsystem::PassesSameEventCooldown(const FString &EventKey,double Now)
{
	if (EventKey.IsEmpty())
	{
		return true;
	}

	if (const double *Found = LastSameEventRealByKey.Find(EventKey))
	{
		if ((Now - *Found) < (double)Tuning.SameEventCooldownSec)
		{
			return false;
		}
	}

	LastSameEventRealByKey.Add(EventKey,Now);
	return true;
}

int32 USynergyPointSubsystem::ConsumePerSecondBudget(int32 ProposedAmount,double Now)
{
	if (ProposedAmount <= 0)
		return 0;

	for (int32 i = RecentGainSlices.Num() - 1; i >= 0; --i)
	{
		if ((Now - RecentGainSlices[i].TimeReal) > 1.0)
		{
			RecentGainSlices.RemoveAt(i);
		}
	}

	int32 Used = 0;
	for (const FRecentGainSlice &Slice : RecentGainSlices)
	{
		Used += Slice.Amount;
	}

	const int32 RemainingBudget = FMath::Max(0,Tuning.SPMaxGainPerSec-Used);
	const int32 Granted = FMath::Clamp(ProposedAmount,0,RemainingBudget);

	if (Granted>0)
	{
		FRecentGainSlice NewSlice;
		NewSlice.TimeReal = Now;
		NewSlice.Amount = Granted;
		RecentGainSlices.Add(NewSlice);
	}

	return Granted;
}

void USynergyPointSubsystem::UpdateReadyState()
{
	const bool bPrevReady = State.bChainReady;
	State.SPCap = Tuning.SPCap;
	State.bChainReady = (State.CurrentSP >= State.SPCap);

	if (bPrevReady != State.bChainReady)
	{
		OnSynergyReadyChanged.Broadcast(State.bChainReady);
	}
}

void USynergyPointSubsystem::ApplyGainEvent(FJRPGSPGainEvent &Event)
{
	if (!CanAcceptGain())
		return;

	const int32 Proposed = Event.BaseAmount + Event.RoleBonusAmount + Event.TacticalBonusAmount;
	
	if (Proposed <= 0)
		return;
	
	const double Now = FPlatformTime::Seconds();
	Event.TimestampReal = Now;
	Event.FinalGrantedAmount =ConsumePerSecondBudget(Proposed, Now);
	if (Event.FinalGrantedAmount <= 0)
	{
		return;
	}

	if (Tuning.bOvercapAllowed)
	{
		State.CurrentSP += Event.FinalGrantedAmount;
	}
	else
	{
		State.CurrentSP = FMath::Min(State.SPCap,State.CurrentSP + Event.FinalGrantedAmount);
	}

	State.LastGainRealTime = Now;
	UpdateReadyState();

	OnSynergyPointChanged.Broadcast(State.CurrentSP, Event.FinalGrantedAmount, Event.Type, Event.ReasonTag);
	OnSynergyGainApplied.Broadcast(Event);
}

void USynergyPointSubsystem::ResetForBattleEnd(FName ReasonTag)
{
	const int32 Prev = State.CurrentSP;

	State.CurrentSP = 0;
	State.LastGainRealTime = FPlatformTime::Seconds();
	
	RecentGainSlices.Reset();
	LastSameEventRealByKey.Reset();
	DamageWindows.Reset();
	
	UpdateReadyState();

	if (Prev > 0)
	{
		OnSynergyPointChanged.Broadcast(State.CurrentSP, -Prev, EJRPGSPEventType::None, ReasonTag);
	}
}

void USynergyPointSubsystem::ResetForChainEnd(FName ReasonTag)
{
	const int32 Prev =State.CurrentSP;

	State.CurrentSP = 0;
	State.LastGainRealTime = FPlatformTime::Seconds();
	RecentGainSlices.Reset();
	UpdateReadyState();

	if (Prev > 0)
	{
		OnSynergyPointChanged.Broadcast(State.CurrentSP,-Prev, EJRPGSPEventType::None,ReasonTag);
	}
}

void USynergyPointSubsystem::HandleBattleEnded(const FBattleSessionSnapshot &, EBattleEndReason)
{
	ResetForBattleEnd("SP.Reset.BattleEnd");
}

void USynergyPointSubsystem::HandleChainEnded(const FChainAttackSnapshot &)
{
	ResetForChainEnd("SP.Reset.ChainEnd");
}

void USynergyPointSubsystem::ReportDamage(AActor *Instigator, AActor *Target,
	float DamageAmount, bool bFromTacticalReservation, FName ReasonTag)
{
	if (!Instigator || !Target || DamageAmount <= 0.f)
		return;

	FJRPGSPGainEvent Evt;
	Evt.Instigator = Instigator;
	Evt.Target = Target;
	Evt.Role = ResolveRoleForActor(Instigator);
	Evt.Type = EJRPGSPEventType::Damage;
	Evt.BaseAmount = 1 + FMath::FloorToInt(DamageAmount / 100.f);
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;

	if (Evt.Role == EJRPGPartyRole::Attacker)
	{
		FDamageWindowRuntime &W = DamageWindows.FindOrAdd(Instigator);
		const double Now = FPlatformTime::Seconds();

		if ((Now - W.WindowStartReal) > (double)Tuning.AttackerDamageWindowSec)
		{
			W.WindowStartReal = Now;
			W.AccDamage = DamageAmount;
		}
		else
		{
			W.AccDamage += DamageAmount;
		}

		const FString CoolKey = FString::Printf(TEXT("SP.Attacker.DamageWindow.%s"), *Instigator->GetName());

		if (W.AccDamage >= Tuning.AttackerDamageWindowThreshold && PassesSameEventCooldown(CoolKey,Now))
		{
			Evt.RoleBonusAmount += Tuning.AttackerDamageWindowBonus;
			W.AccDamage = 0.f;
			W.WindowStartReal = Now;
		}
	}

	Evt.TacticalBonusAmount = ComputeTacticalBonus(Evt.RoleBonusAmount, bFromTacticalReservation);
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportHeal(AActor *Instigator, AActor *Target,
	float HealAmount, float TargetHPRatioBefore, bool bFromTacticalReservation, FName ReasonTag)
{
	if (!Instigator || !Target || HealAmount <= 0.f)
		return;

	FJRPGSPGainEvent Evt;
	Evt.Instigator = Instigator;
	Evt.Target = Target;
	Evt.Role = ResolveRoleForActor(Instigator);
	Evt.Type = EJRPGSPEventType::Heal;
	Evt.BaseAmount = 1 + FMath::FloorToInt(HealAmount / 120.f);
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;

	if (Evt.Role == EJRPGPartyRole::Supporter && TargetHPRatioBefore <= Tuning.SupporterCriticalHealThreshold)
	{
		Evt.RoleBonusAmount += Tuning.SupporterCriticalHealBonus;
	}

	Evt.TacticalBonusAmount = ComputeTacticalBonus(Evt.RoleBonusAmount, bFromTacticalReservation);
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportBreak(AActor*Instigator, AActor*Target,
	float BreakAmount, bool bTriggeredStun, bool bFromTacticalReservation, FName ReasonTag)
{
	if (!Instigator || !Target) return;
	if (BreakAmount <= 0.f && !bTriggeredStun) return;

	FJRPGSPGainEvent Evt;
	Evt.Instigator = Instigator;
	Evt.Target = Target;
	Evt.Role = ResolveRoleForActor(Instigator);
	Evt.Type = EJRPGSPEventType::Break;
	Evt.BaseAmount = (BreakAmount > 0.f) ? 1 : 0;
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;

	if (Evt.Role == EJRPGPartyRole::Attacker)
	{
		if (BreakAmount > 0.f)
		{
			Evt.RoleBonusAmount += FMath::FloorToInt(BreakAmount * Tuning.AttackerBreakContributionCoeff);
		}

		if (bTriggeredStun)
		{
			const double Now = FPlatformTime::Seconds();
			const FString CoolKey = FString::Printf(TEXT("SP.Attacker.Stun.%s.%s"),
							*Instigator->GetName(),
							*Target->GetName());

			if (PassesSameEventCooldown(CoolKey,Now))
			{
				Evt.RoleBonusAmount += Tuning.AttackerStunTriggerBonus;
			}
		}
	}

	Evt.TacticalBonusAmount =ComputeTacticalBonus(Evt.RoleBonusAmount, bFromTacticalReservation);
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportBuff(AActor *Instigator, AActor *Target, FName BuffId,
	bool bFromTacticalReservation, FName ReasonTag)
{
	if (!Instigator || !Target || BuffId.IsNone()) return;

	FJRPGSPGainEvent Evt;
	Evt.Instigator = Instigator;
	Evt.Target = Target;
	Evt.Role = ResolveRoleForActor(Instigator);
	Evt.Type = EJRPGSPEventType::Buff;
	Evt.BaseAmount = 1;
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;

	if (Evt.Role == EJRPGPartyRole::Supporter && IsAlly(Instigator,Target))
	{
		const double Now = FPlatformTime::Seconds();
		const FString CoolKey = FString::Printf(TEXT("SP.Supporter.Buff.%s.%s"),
					*Instigator->GetName(),
					*BuffId.ToString());

		if (PassesSameEventCooldown(CoolKey, Now))
		{
			Evt.RoleBonusAmount += Tuning.SupporterBuffUptimeBonus;
		}
	}

	Evt.TacticalBonusAmount =ComputeTacticalBonus(Evt.RoleBonusAmount, bFromTacticalReservation);
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportDebuff(AActor *Instigator, AActor *Target, FName DebuffId,
	bool bFromTacticalReservation, FName ReasonTag)
{
	if (!Instigator || !Target || DebuffId.IsNone()) 
		return;
 
	FJRPGSPGainEvent Evt;
	Evt.Instigator = Instigator;
	Evt.Target = Target;
	Evt.Role = ResolveRoleForActor(Instigator);
	Evt.Type = EJRPGSPEventType::Debuff;
	Evt.BaseAmount = 1;
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;
 
	// 문서상 Debuff는 기본 획득에 포함되지만 별도 롤 보너스 계수는 고정되어 있지 않으므로
	// 현재는 Base Gain만 준다.
	Evt.TacticalBonusAmount = ComputeTacticalBonus(Evt.RoleBonusAmount, bFromTacticalReservation);
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportCleanse(AActor *Instigator, AActor *Target,
	int32 RemovedCount, bool bRemovedCriticalCC, bool bFromTacticalReservation, FName ReasonTag)
{
	if (!Instigator || !Target || RemovedCount <= 0)
		return;
 
	FJRPGSPGainEvent Evt;
	Evt.Instigator = Instigator;
	Evt.Target = Target;
	Evt.Role = ResolveRoleForActor(Instigator);
	Evt.Type = EJRPGSPEventType::Cleanse;
	Evt.BaseAmount = 1;
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;
 
	if (Evt.Role == EJRPGPartyRole::Supporter && bRemovedCriticalCC)
	{
		Evt.RoleBonusAmount += Tuning.SupporterCleanseBonus;
	}
 
	Evt.TacticalBonusAmount = ComputeTacticalBonus(Evt.RoleBonusAmount,bFromTacticalReservation);
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportThreatOutcome(AActor* Instigator, AActor* EnemyOwner,
	float ThreatDelta, bool bBecameTopThreat, bool bRescuedAlly, bool bFromTacticalReservation, FName ReasonTag)
{
	if (!Instigator||!EnemyOwner||ThreatDelta<=0.f)return;
 
	FJRPGSPGainEvent Evt;
	Evt.Instigator = Instigator;
	Evt.Target = EnemyOwner;
	Evt.Role = ResolveRoleForActor(Instigator);
	Evt.Type = EJRPGSPEventType::Taunt;
	Evt.BaseAmount = 1;
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;
 
	if (Evt.Role == EJRPGPartyRole::Defender)
	{
		if (bRescuedAlly)
		{
			Evt.RoleBonusAmount += Tuning.DefenderAggroRescueBonus;
		}
		else if (bBecameTopThreat)
		{
			Evt.RoleBonusAmount += Tuning.DefenderAggroHoldBonus;
		}
	}
	
	Evt.TacticalBonusAmount = ComputeTacticalBonus(Evt.RoleBonusAmount,bFromTacticalReservation);
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportAggroHold(AActor* Defender, AActor* EnemyOwner,
	bool bFromTacticalReservation,FName ReasonTag)
{
	if (!Defender || !EnemyOwner)
		return;
 
	const double Now = FPlatformTime::Seconds();
	const FString CoolKey = FString::Printf(TEXT("SP.Defender.Hold.%s.%s"),
			*Defender->GetName(),
			*EnemyOwner->GetName());
 
	if (!PassesSameEventCooldown(CoolKey, Now))
		return;
 	
 
	FJRPGSPGainEvent Evt;
	Evt.Instigator = Defender;
	Evt.Target = EnemyOwner;
	Evt.Role = ResolveRoleForActor(Defender);
	Evt.Type = EJRPGSPEventType::Taunt;
	Evt.BaseAmount = 0;
	Evt.RoleBonusAmount = (Evt.Role == EJRPGPartyRole::Defender) ? Tuning.DefenderAggroHoldBonus : 0;
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;
	Evt.TacticalBonusAmount = ComputeTacticalBonus(Evt.RoleBonusAmount, bFromTacticalReservation);
 
	ApplyGainEvent(Evt);
}

void USynergyPointSubsystem::ReportPartyProtect(AActor* Defender,AActor* ProtectedTarget,
	bool bFromTacticalReservation,FName ReasonTag)
{
	if (!Defender || !ProtectedTarget)
		return;
 
	FJRPGSPGainEvent Evt;
	Evt.Instigator = Defender;
	Evt.Target = ProtectedTarget;
	Evt.Role = ResolveRoleForActor(Defender);
	Evt.Type = EJRPGSPEventType::Protect;
	Evt.BaseAmount = 1;
	Evt.RoleBonusAmount = (Evt.Role == EJRPGPartyRole::Defender) ? Tuning.DefenderPartyProtectBonus : 0;
	Evt.bFromTacticalReservation = bFromTacticalReservation;
	Evt.ReasonTag = ReasonTag;
	Evt.TacticalBonusAmount = ComputeTacticalBonus(Evt.RoleBonusAmount, bFromTacticalReservation);
 
	ApplyGainEvent(Evt);
}