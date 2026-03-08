#include "Combat/Infrastructure/CombatSynergyPointSubsystem.h"

#include "Combat/SP/SynergyPointSettingsDataAsset.h"
#include "Combat/Core/CombatRoleComponent.h"
#include "Combat/Infrastructure/CombatBattleSessionSubsystem.h"

#include "Engine/World.h"

void UCombatSynergyPointSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// SettingsAsset가 외부에서 주입되지 않으면 기본값 사용(DA는 프로젝트에서 지정 권장)
	Settings = SettingsAsset ? SettingsAsset->Settings : FSynergyPointSettings();

	State.SPCap = Settings.SPCap;
	State.CurrentSP = 0;
	State.bChainReady = false;
	State.LastGainRealTime = 0.0;

	GainWindowStartReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	GainWindowAccum = 0;

	LastEventRealTimeByKey.Reset();
	AggroHoldByEnemy.Reset();
	LastBreakInstigatorByVictim.Reset();
	DamageWindowByInstigator.Reset();
}

void UCombatSynergyPointSubsystem::Deinitialize()
{
	LastEventRealTimeByKey.Reset();
	AggroHoldByEnemy.Reset();
	LastBreakInstigatorByVictim.Reset();
	DamageWindowByInstigator.Reset();
	Super::Deinitialize();
}

bool UCombatSynergyPointSubsystem::IsBattleActive() const
{
	if (UWorld* W = GetWorld())
	{
		if (UCombatBattleSessionSubsystem* Battle = W->GetSubsystem<UCombatBattleSessionSubsystem>())
		{
			return Battle->IsCombatRunning();
		}
	}
	return false;
}

int32 UCombatSynergyPointSubsystem::ComputeBaseGain(ESPEventType Type) const
{
	switch (Type)
	{
	case ESPEventType::Damage:  return Settings.BaseGain_Damage;
	case ESPEventType::Heal:    return Settings.BaseGain_Heal;
	case ESPEventType::Debuff:  return Settings.BaseGain_Debuff;
	case ESPEventType::Buff:    return Settings.BaseGain_Buff;
	case ESPEventType::Cleanse: return Settings.BaseGain_Cleanse;
	case ESPEventType::Taunt:   return Settings.BaseGain_Taunt;
	case ESPEventType::Break:   return Settings.BaseGain_Break;
	default:
		// RoleBonus 전용 이벤트들은 base 0
		return 0;
	}
}

ECombatRole UCombatSynergyPointSubsystem::GetRoleOf(AActor* Actor) const
{
	if (!Actor) return ECombatRole::Unknown;
	if (const UCombatRoleComponent* R = Actor->FindComponentByClass<UCombatRoleComponent>())
		return R->GetRole();

	// fallback: tag 기반(팀이 선호하면 사용)
	if (Actor->ActorHasTag("Role.Defender")) return ECombatRole::Defender;
	if (Actor->ActorHasTag("Role.Attacker")) return ECombatRole::Attacker;
	if (Actor->ActorHasTag("Role.Supporter")) return ECombatRole::Supporter;

	return ECombatRole::Unknown;
}

FName UCombatSynergyPointSubsystem::MakeCooldownKey(const FSPGainEvent& E, FName ReasonTag) const
{
	// 동일 이벤트 억제: (Type + Role + Instigator) 기준
	const FString Key = FString::Printf(TEXT("SPCD.%d.%d.%s.%s"),
		(int32)E.Type,
		(int32)E.Role,
		*GetNameSafe(E.Instigator),
		*ReasonTag.ToString());
	return FName(*Key);
}

bool UCombatSynergyPointSubsystem::PassSameEventCooldown(const FSPGainEvent& E, FName ReasonTag)
{
	// 반복 억제는 “RoleBonus가 큰 이벤트” 위주로 적용(문서 5.4 예: AggroHold 3초마다 1회)
	const bool bCooldownType =
		(E.Type == ESPEventType::DefenderAggroHold) ||
		(E.Type == ESPEventType::DefenderAggroRescue) ||
		(E.Type == ESPEventType::AttackerStunTrigger) ||
		(E.Type == ESPEventType::AttackerDamageWindow) ||
		(E.Type == ESPEventType::SupporterCriticalHeal) ||
		(E.Type == ESPEventType::Cleanse) ||
		(E.Type == ESPEventType::SupporterBuffUptime);

	if (!bCooldownType) return true;

	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	const FName Key = MakeCooldownKey(E, ReasonTag);

	if (const double* Last = LastEventRealTimeByKey.Find(Key))
	{
		if ((Now - *Last) < Settings.SameEventCooldownSec)
			return false;
	}

	LastEventRealTimeByKey.Add(Key, Now);
	return true;
}

int32 UCombatSynergyPointSubsystem::ApplyTacticalBonus(int32 RoleBonus, bool bFromTactical) const
{
	if (!bFromTactical || RoleBonus <= 0) return 0;

	// 문서 5.3.3: Multiplier 방식 우선
	const int32 Multed = FMath::RoundToInt((float)RoleBonus * Settings.TacticalRoleBonusMultiplier);
	const int32 Extra = FMath::Max(0, Multed - RoleBonus);

	// Flat bonus 옵션(있으면 추가)
	return Extra + Settings.TacticalFlatBonus;
}

int32 UCombatSynergyPointSubsystem::ApplyPerSecondCap(int32 ProposedGain, double NowReal)
{
	// 문서 5.4: 초당 획득량 상한
	if (Settings.SPMaxGainPerSec <= 0) return ProposedGain;

	if ((NowReal - GainWindowStartReal) >= 1.0)
	{
		GainWindowStartReal = NowReal;
		GainWindowAccum = 0;
	}

	const int32 Remaining = FMath::Max(0, Settings.SPMaxGainPerSec - GainWindowAccum);
	const int32 Clamped = FMath::Min(ProposedGain, Remaining);
	GainWindowAccum += Clamped;
	return Clamped;
}

int32 UCombatSynergyPointSubsystem::ComputeRoleBonus(FSPGainEvent& InOutEvent)
{
	// Outcome 기반 판정(문서 5.2)
	switch (InOutEvent.Role)
	{
	case ECombatRole::Attacker:
	{
		if (InOutEvent.Type == ESPEventType::Break)
		{
			// 브레이크 기여(문서 5.2.2)
			LastBreakInstigatorByVictim.Add(InOutEvent.Target, InOutEvent.Instigator);

			const float Raw = InOutEvent.OutcomeValue * Settings.AttackerBreakContributionCoeff;
			return FMath::Clamp((int32)FMath::RoundToInt(Raw), 0, Settings.AttackerBreakContributionCap);
		}

		if (InOutEvent.Type == ESPEventType::Damage)
		{
			// 유효 피해량 윈도우(문서 5.2.2 DamageWindow)
			if (!InOutEvent.Instigator) return 0;

			FDamageWindow& W = DamageWindowByInstigator.FindOrAdd(InOutEvent.Instigator);
			const double Now = InOutEvent.TimestampReal;

			if (W.WindowStartReal <= 0.0 || (Now - W.WindowStartReal) > Settings.AttackerDamageWindowSec)
			{
				W.WindowStartReal = Now;
				W.AccumDamage = 0.f;
			}

			W.AccumDamage += InOutEvent.OutcomeValue;

			if (W.AccumDamage >= Settings.AttackerDamageWindowThreshold)
			{
				// 이벤트 쿨다운(동일 이벤트 억제)
				FSPGainEvent CooldownEvent = InOutEvent;
				CooldownEvent.Type = ESPEventType::AttackerDamageWindow;
				if (!PassSameEventCooldown(CooldownEvent, "Role.AttackerDamageWindow"))
					return 0;

				// 보너스 지급 후 리셋
				W.AccumDamage = 0.f;
				W.WindowStartReal = Now;
				return Settings.AttackerDamageWindowBonus;
			}
		}
		return 0;
	}

	case ECombatRole::Supporter:
	{
		if (InOutEvent.Type == ESPEventType::Heal)
		{
			// 위기 회복(문서 5.2.3 CriticalHeal)
			if (InOutEvent.TargetHPBeforeRatio < Settings.SupporterCriticalHealHPThreshold)
			{
				FSPGainEvent CooldownEvent = InOutEvent;
				CooldownEvent.Type = ESPEventType::SupporterCriticalHeal;
				if (!PassSameEventCooldown(CooldownEvent, "Role.SupporterCriticalHeal"))
					return 0;

				return Settings.SupporterCriticalHealBonus;
			}
		}

		if (InOutEvent.Type == ESPEventType::Cleanse)
		{
			FSPGainEvent CooldownEvent = InOutEvent;
			CooldownEvent.Type = ESPEventType::Cleanse;
			if (!PassSameEventCooldown(CooldownEvent, "Role.SupporterCleanse"))
				return 0;

			return Settings.SupporterCleanseBonus;
		}

		if (InOutEvent.Type == ESPEventType::SupporterBuffUptime)
		{
			FSPGainEvent CooldownEvent = InOutEvent;
			if (!PassSameEventCooldown(CooldownEvent, "Role.SupporterBuffUptime"))
				return 0;

			return Settings.SupporterBuffUptimeBonus;
		}

		return 0;
	}

	case ECombatRole::Defender:
	default:
		// Defender는 NotifyEnemyTargetChanged / HoldTimer에서 RoleBonus가 발생(문서 5.2.1)
		return 0;
	}
}

void UCombatSynergyPointSubsystem::ApplyGainInternal(FSPGainEvent& Snapshot, int32 TotalGain, FName ReasonTag)
{
	const int32 Prev = State.CurrentSP;

	if (!Settings.bOvercapAllowed)
	{
		State.CurrentSP = FMath::Min(State.SPCap, State.CurrentSP + TotalGain);
	}
	else
	{
		State.CurrentSP += TotalGain;
	}

	State.LastGainRealTime = Snapshot.TimestampReal;

	OnSynergyPointGained.Broadcast(Snapshot);
	OnSynergyPointChanged.Broadcast(State.CurrentSP, State.CurrentSP - Prev, ReasonTag);

	UpdateReady();
}

void UCombatSynergyPointSubsystem::UpdateReady()
{
	const bool bPrev = State.bChainReady;
	State.bChainReady = (State.CurrentSP >= State.SPCap);

	if (bPrev != State.bChainReady)
	{
		OnSynergyReadyChanged.Broadcast(State.bChainReady);
	}
}

bool UCombatSynergyPointSubsystem::SubmitGainEvent(const FSPGainEvent& InEvent, FName ReasonTag)
{
	// 문서 3.2: 세션 Active 에서만 획득
	if (!IsBattleActive())
		return false;

	if (!InEvent.Instigator)
		return false;

	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;

	FSPGainEvent Snapshot = InEvent;
	Snapshot.TimestampReal = Now;

	// Role은 입력이 없으면 자동 판정
	if (Snapshot.Role == ECombatRole::Unknown)
	{
		Snapshot.Role = GetRoleOf(Snapshot.Instigator);
	}

	// BaseGain
	Snapshot.BaseAmount = ComputeBaseGain(Snapshot.Type);

	// RoleBonus (Outcome 기반)
	Snapshot.RoleBonusAmount = ComputeRoleBonus(Snapshot);

	// TacticalBonus (문서 5.3: RoleBonus에 배율/플랫)
	Snapshot.TacticalBonusAmount = ApplyTacticalBonus(Snapshot.RoleBonusAmount, Snapshot.bFromTacticalReservation);

	// 동일 이벤트 쿨다운(문서 5.4)
	// - 주로 RoleBonus 이벤트에 걸리지만, 여기서 한번 더 안전하게 적용(특정 타입은 PassSameEventCooldown 내부)
	// - BaseGain은 기본적으로 억제하지 않음

	int32 TotalGain = Snapshot.BaseAmount + Snapshot.RoleBonusAmount + Snapshot.TacticalBonusAmount;
	if (TotalGain <= 0) return false;

	// 초당 상한(문서 5.4)
	TotalGain = ApplyPerSecondCap(TotalGain, Now);
	if (TotalGain <= 0) return false;

	ApplyGainInternal(Snapshot, TotalGain, ReasonTag.IsNone() ? FName("SP.Gain") : ReasonTag);
	return true;
}

void UCombatSynergyPointSubsystem::Reset(FName ReasonTag)
{
	const int32 Prev = State.CurrentSP;
	State.CurrentSP = 0;
	State.LastGainRealTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;

	OnSynergyPointChanged.Broadcast(State.CurrentSP, -Prev, ReasonTag.IsNone() ? FName("SP.Reset") : ReasonTag);

	const bool bPrevReady = State.bChainReady;
	State.bChainReady = false;
	if (bPrevReady)
	{
		OnSynergyReadyChanged.Broadcast(false);
	}

	// 제한/캐시 초기화
	GainWindowStartReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	GainWindowAccum = 0;

	LastEventRealTimeByKey.Reset();
	AggroHoldByEnemy.Reset();
	LastBreakInstigatorByVictim.Reset();
	DamageWindowByInstigator.Reset();
}

void UCombatSynergyPointSubsystem::ResetForChainEnd()
{
	Reset("SP.Reset.ChainEnd");
}

void UCombatSynergyPointSubsystem::ResetForBattleEnd()
{
	Reset("SP.Reset.BattleEnd");
}

void UCombatSynergyPointSubsystem::NotifyEnemyTargetChanged(AActor* Enemy, AActor* OldTarget, AActor* NewTarget)
{
	if (!Enemy || !NewTarget) return;

	// DefenderAggroRescue: 아군이 타겟이던 상황에서 Defender가 타겟을 뺏어옴(문서 5.2.1)
	const ECombatRole NewRole = GetRoleOf(NewTarget);
	if (NewRole == ECombatRole::Defender)
	{
		const ECombatRole OldRole = GetRoleOf(OldTarget);

		if (OldTarget && OldTarget != NewTarget && OldRole != ECombatRole::Defender)
		{
			FSPGainEvent Ev;
			Ev.Instigator = NewTarget;
			Ev.Target = Enemy;
			Ev.Role = ECombatRole::Defender;
			Ev.Type = ESPEventType::DefenderAggroRescue;
			Ev.OutcomeValue = 1.f;

			// 쿨다운 적용
			if (PassSameEventCooldown(Ev, "Role.DefenderAggroRescue"))
			{
				// RoleBonus를 “직접 이벤트”로 주기 위해: base는 0, role bonus는 settings 값으로 처리
				Ev.BaseAmount = 0;
				Ev.RoleBonusAmount = Settings.DefenderAggroRescueBonus;
				Ev.TacticalBonusAmount = ApplyTacticalBonus(Ev.RoleBonusAmount, Ev.bFromTacticalReservation);

				int32 Total = Ev.RoleBonusAmount + Ev.TacticalBonusAmount;
				Total = ApplyPerSecondCap(Total, GetWorld()->GetRealTimeSeconds());
				if (Total > 0 && IsBattleActive())
				{
					Ev.TimestampReal = GetWorld()->GetRealTimeSeconds();
					ApplyGainInternal(Ev, Total, "Role.DefenderAggroRescue");
				}
			}
		}

		// AggroHold 추적 시작/갱신
		FAggroHoldTrack& Track = AggroHoldByEnemy.FindOrAdd(Enemy);
		Track.CurrentTarget = NewTarget;
		Track.HoldStartReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	}
	else
	{
		// 타겟이 Defender가 아니면 해당 Enemy 트랙 리셋
		if (FAggroHoldTrack* Track = AggroHoldByEnemy.Find(Enemy))
		{
			Track->CurrentTarget = NewTarget;
			Track->HoldStartReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
		}
	}
}

void UCombatSynergyPointSubsystem::NotifyVictimStunned(AActor* Victim)
{
	if (!Victim) return;

	// 마지막 브레이크 기여자에게 StunTrigger 보너스(문서 5.2.2)
	TWeakObjectPtr<AActor>* Last = LastBreakInstigatorByVictim.Find(Victim);
	AActor* Instigator = Last ? Last->Get() : nullptr;
	if (!Instigator) return;

	FSPGainEvent Ev;
	Ev.Instigator = Instigator;
	Ev.Target = Victim;
	Ev.Role = ECombatRole::Attacker;
	Ev.Type = ESPEventType::AttackerStunTrigger;
	Ev.OutcomeValue = 1.f;

	if (!PassSameEventCooldown(Ev, "Role.AttackerStunTrigger"))
		return;

	Ev.BaseAmount = 0;
	Ev.RoleBonusAmount = Settings.AttackerStunTriggerBonus;
	Ev.TacticalBonusAmount = ApplyTacticalBonus(Ev.RoleBonusAmount, Ev.bFromTacticalReservation);

	int32 Total = Ev.RoleBonusAmount + Ev.TacticalBonusAmount;
	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	Total = ApplyPerSecondCap(Total, Now);

	if (Total > 0 && IsBattleActive())
	{
		Ev.TimestampReal = Now;
		ApplyGainInternal(Ev, Total, "Role.AttackerStunTrigger");
	}
}

void UCombatSynergyPointSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Defender AggroHold: Enemy의 타겟이 Defender로 유지되면 보너스(문서 5.2.1)
	if (!IsBattleActive()) return;

	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;

	for (auto& It : AggroHoldByEnemy)
	{
		AActor* Enemy = It.Key.Get();
		FAggroHoldTrack& Track = It.Value;

		AActor* Tgt = Track.CurrentTarget.Get();
		if (!Enemy || !Tgt) continue;

		if (GetRoleOf(Tgt) != ECombatRole::Defender) continue;

		const double Held = Now - Track.HoldStartReal;
		if (Held >= Settings.DefenderAggroHoldWindowSec)
		{
			FSPGainEvent Ev;
			Ev.Instigator = Tgt;
			Ev.Target = Enemy;
			Ev.Role = ECombatRole::Defender;
			Ev.Type = ESPEventType::DefenderAggroHold;
			Ev.OutcomeValue = (float)Held;

			// 동일 이벤트 쿨다운(문서 5.4 예시)
			if (!PassSameEventCooldown(Ev, "Role.DefenderAggroHold"))
			{
				// 쿨다운 중에도 HoldStart를 갱신하지 않으면 “쿨다운 끝나자마자 연속 지급”이 될 수 있어
				// 그래서 지급 타이밍마다 HoldStart를 새로 시작
				Track.HoldStartReal = Now;
				continue;
			}

			Ev.BaseAmount = 0;
			Ev.RoleBonusAmount = Settings.DefenderAggroHoldBonus;
			Ev.TacticalBonusAmount = ApplyTacticalBonus(Ev.RoleBonusAmount, Ev.bFromTacticalReservation);

			int32 Total = Ev.RoleBonusAmount + Ev.TacticalBonusAmount;
			Total = ApplyPerSecondCap(Total, Now);

			if (Total > 0)
			{
				Ev.TimestampReal = Now;
				ApplyGainInternal(Ev, Total, "Role.DefenderAggroHold");
			}

			// 다음 윈도우 시작
			Track.HoldStartReal = Now;
		}
	}
}