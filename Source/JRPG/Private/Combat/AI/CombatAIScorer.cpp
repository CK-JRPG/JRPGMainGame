// Source/JRPGCombat/Private/Combat/AI/CombatAIScorer.cpp
#include "Combat/AI/CombatAIScorer.h"
#include "Combat/AI/CombatAIContext.h"
#include "Combat/AI/CombatAIPresetAsset.h"

#include "Combat/Skills/SkillComponent.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Status/StatusComponent.h"

static FString ScoreReason(const TCHAR *Msg, float Score)
{
	return FString::Printf(TEXT("%s (%.2f)"),Msg,Score);
}

bool FCombatAIScorer::CanUseAnySkill(const UCombatAIContext &Ctx, const TArray<FName> &SkillIds, FName &OutChosen)
{
	OutChosen = NAME_None;
	if (!Ctx.Skill) return false;

	for (const FName &Id : SkillIds)
	{
		if (Id.IsNone()) continue;
		FGameplayTag FailReason;
		if (Ctx.Skill->CanUseSkill(Id, FailReason))
		{
			OutChosen = Id;
			return true;
		}
	}
	return false;
}

AActor* FCombatAIScorer::FindMostCriticalAlly(const UCombatAIContext &Ctx, float HpThreshold)
{
	TArray<AActor*> Party;
	Ctx.GetPartyMembers(Party);

	AActor *Best = nullptr;
	float BestHP =2.0f;

	for (AActor *Ally : Party)
	{
		if (!Ally) continue;
		UHPComponent*HP = Ally->FindComponentByClass<UHPComponent>();
		if (!HP || !HP->IsAlive()) continue;

		const float P = HP->GetHPPercent();
		if (P < HpThreshold && P < BestHP)
		{
			BestHP = P;
			Best =Ally;
		}
	}
	return Best;
}

bool FCombatAIScorer::AllyHasCC(const UCombatAIContext &Ctx, AActor* &OutAllyWithCC)
{
	OutAllyWithCC = nullptr;

	TArray<AActor*> Party;
	Ctx.GetPartyMembers(Party);

	for (AActor *Ally : Party)
	{
		if (!Ally) continue;
		
		UStatusComponent *S = Ally->FindComponentByClass<UStatusComponent>();
		UHPComponent *HP = Ally->FindComponentByClass<UHPComponent>();
		if (!HP || !HP->IsAlive()) continue;

		if (S && S->HasDangerousCC())
		{
			OutAllyWithCC = Ally;
			return true;
		}
	}
	return false;
}

float FCombatAIScorer::ApplyReservationHoldPenalty(
	const UCombatAIContext &Ctx,
	const UCombatAIPresetAsset &PresetAsset,
	ECombatAIPreset Preset,
	float RawScore)
{
	// 전술 예약이 잡혀있으면(특히 수비 프리셋) AP/쿨을 낭비하지 말고 “예약 성공”을 우선하도록
	// 다른 액션 점수에 페널티를 부여
	if (!Ctx.Reservation.bHasReservation) return RawScore;

	const float Hold = PresetAsset.GetReservedApHoldStrength(Preset);// 0~1
	return RawScore * (1.0f - 0.35f * Hold);
}

FCombatAIAction FCombatAIScorer::ChoosePartyAction(
	const UCombatAIContext &Ctx,
	const UCombatAIPresetAsset &PresetAsset,
	ECombatPartyRole Role,
	ECombatAIPreset Preset)
{
	FCombatAIAction Out;

	if (!Ctx.bSessionActive || !Ctx.IsAlive())
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.Score = 0.f;
		Out.DebugReason = TEXT("Session Inactive Or Dead");
		return Out;
	}

	// 입력 잠금/체인 중에는 파티 AI “행동” 자체는 보류하는 게 안전(체인은 별도 시퀀스 전투)
	if (Ctx.bInputLocked || Ctx.bChainActive)
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.Score = 0.f;
		Out.DebugReason = TEXT("Input Locked Or ChainActive");
		return Out;
	}

	// CC면 행동불가(문서 규약)
	if (Ctx.IsCCBlocked())
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.Score = 0.f;
		Out.DebugReason = TEXT("CCBlocked");
		return Out;
	}

	const FCombatAIPartyTuning &Tuning = PresetAsset.GetPartyTuning(Role);
	const FCombatAIWeights W = PresetAsset.GetEffectiveWeights(Role, Preset);

	// 역할별 “우선순위”는 후보 점수로 강제
	// Supporter: HP<30% 힐, CC정화, 버프, 디버프, 여유 시 공격/브레이크 (AI 문서)
	if (Role == ECombatPartyRole::Supporter)
	{
		const float HealTh = PresetAsset.GetHealThreshold(Preset);

		// 1) 위기 힐
		if (AActor *Critical = FindMostCriticalAlly(Ctx, HealTh))
		{
			FName HealSkill;
			if (CanUseAnySkill(Ctx, Tuning.Skills.HealSkills,HealSkill))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = HealSkill;
				Out.Target = Critical;
				Out.Score = 1000.f*W.Heal;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, Out.Score);
				Out.DebugReason =ScoreReason(TEXT("Supporter : CriticalHeal"), Out.Score);
				return Out;
			}
		}

		// 2) 정화
		AActor *AllyWithCC = nullptr;
		if (AllyHasCC(Ctx,AllyWithCC))
		{
			FName CleanseSkill;
			if (CanUseAnySkill(Ctx, Tuning.Skills.CleanseSkills,CleanseSkill))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = CleanseSkill;
				Out.Target = AllyWithCC;
				Out.Score = 900.f * W.Cleanse;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, Out.Score);
				Out.DebugReason = ScoreReason(TEXT("Supporter : Cleanse"), Out.Score);
				return Out;
			}
		}

		// 3) 버프 유지(간단: 버프 스킬 사용 가능이면 우선)
		{
			FName BuffSkill;
			if (CanUseAnySkill(Ctx, Tuning.Skills.BuffSkills,BuffSkill))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = BuffSkill;
				Out.Target = Ctx.Owner.Get();// Self or Party TargetType은 Skill 시스템이 처리
				Out.Score = 400.f * W.Buff;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, Out.Score);
				Out.DebugReason = ScoreReason(TEXT("Supporter : Buff"), Out.Score);
				return Out;
			}
		}

		// 4) 적 디버프 유지
		AActor*Primary = Ctx.GetPrimaryTarget();
		if (Primary)
		{
			FName DebuffSkill;
			if (CanUseAnySkill(Ctx,Tuning.Skills.DebuffSkills,DebuffSkill))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = DebuffSkill;
				Out.Target = Primary;
				Out.Score = 320.f * W.Debuff;
				Out.Score =ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, Out.Score);
				Out.DebugReason =ScoreReason(TEXT("Supporter : Debuff"), Out.Score);
				return Out;
			}
		}

		// 5) 여유 시 브레이크 보조 → 6) 공격
		if (Primary)
		{
			FName BreakSkill;
			if (CanUseAnySkill(Ctx,Tuning.Skills.BreakSkills, BreakSkill))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = BreakSkill;
				Out.Target = Primary;
				Out.Score = 220.f * W.Break;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, Out.Score);
				Out.DebugReason = ScoreReason(TEXT("Supporter : BreakAssist"), Out.Score);
				return Out;
			}

			FName DpsSkill;
			if (CanUseAnySkill(Ctx, Tuning.Skills.DpsSkills, DpsSkill))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = DpsSkill;
				Out.Target = Primary;
				Out.Score = 180.f * W.Attack;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, Out.Score);
				Out.DebugReason = ScoreReason(TEXT("Supporter : AttackSkill"), Out.Score);
				return Out;
			}

			Out.Type = ECombatAIActionType::BasicAttack;
			Out.Target = Primary;
			Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, 120.f);
			Out.DebugReason = ScoreReason(TEXT("Supporter : BasicAttack"), Out.Score);
			return Out;
		}

		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Supporter : NoTarget");
		return Out;
	}

	// Defender: Threat 유지(도발/위협), 파티 보호, 브레이크 보조, 공격
	if (Role == ECombatPartyRole::Defender)
	{
		AActor *Primary = Ctx.GetPrimaryTarget();
		if (Primary)
		{
			FName Taunt;
			if (CanUseAnySkill(Ctx, Tuning.Skills.TauntThreatSkills,Taunt))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = Taunt;
				Out.Target = Primary;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset,600.f * W.TauntThreat);
				Out.DebugReason = ScoreReason(TEXT("Defender : TauntThreat"), Out.Score);
				return Out;
			}

			FName Protect;
			if (CanUseAnySkill(Ctx, Tuning.Skills.ProtectSkills,Protect))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = Protect;
				Out.Target = Ctx.Owner.Get();
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset,420.f * W.Protect);
				Out.DebugReason = ScoreReason(TEXT("Defender : Protect"), Out.Score);
				return Out;
			}

			FName Break;
			if (CanUseAnySkill(Ctx,Tuning.Skills.BreakSkills,Break))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = Break;
				Out.Target = Primary;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset,240.f * W.Break);
				Out.DebugReason = ScoreReason(TEXT("Defender : BreakAssist"), Out.Score);
				return Out;
			}

			FName Dps;
			if (CanUseAnySkill(Ctx, Tuning.Skills.DpsSkills, Dps))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = Dps;
				Out.Target = Primary;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset, 200.f * W.Attack);
				Out.DebugReason = ScoreReason(TEXT("Defender : AttackSkill"), Out.Score);
				return Out;
			}

			Out.Type = ECombatAIActionType::BasicAttack;
			Out.Target = Primary;
			Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset,140.f);
			Out.DebugReason = ScoreReason(TEXT("Defender : BasicAttack"), Out.Score);
			return Out;
		}

		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Defender:NoTarget");
		return Out;
	}

	// Attacker: 브레이크/디버프(초기), 고DPS, 브레이크 임계 근접 시 브레이크 우선, 기본 공격
	{
		AActor *Primary = Ctx.GetPrimaryTarget();
		if (Primary)
		{
			FName Break;
			if (CanUseAnySkill(Ctx, Tuning.Skills.BreakSkills,Break))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = Break;
				Out.Target = Primary;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset,Preset, 520.f * W.Break);
				Out.DebugReason = ScoreReason(TEXT("Attacker:Break"), Out.Score);
				return Out;
			}

			FName Debuff;
			if (CanUseAnySkill(Ctx, Tuning.Skills.DebuffSkills,Debuff))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = Debuff;
				Out.Target = Primary;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset,380.f * W.Debuff);
				Out.DebugReason = ScoreReason(TEXT("Attacker:Debuff"), Out.Score);
				return Out;
			}

			FName Dps;
			if (CanUseAnySkill(Ctx, Tuning.Skills.DpsSkills, Dps))
			{
				Out.Type = ECombatAIActionType::UseSkill;
				Out.SkillId = Dps;
				Out.Target = Primary;
				Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset, Preset,460.f * W.Attack);
				Out.DebugReason = ScoreReason(TEXT("Attacker : DpsSkill"), Out.Score);
				return Out;
			}

			Out.Type = ECombatAIActionType::BasicAttack;
			Out.Target = Primary;
			Out.Score = ApplyReservationHoldPenalty(Ctx, PresetAsset,Preset,220.f);
			Out.DebugReason = ScoreReason(TEXT("Attacker : BasicAttack"), Out.Score);
			return Out;
		}

		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Attacker : NoTarget");
		return Out;
	}
}

FCombatAIAction FCombatAIScorer::ChooseEnemyAction(
	const UCombatAIContext &Ctx,
	const UCombatAIPresetAsset &PresetAsset,
	EEnemyCombatAIState State,
	AActor *CurrentTarget)
{
	FCombatAIAction Out;

	if (!Ctx.bSessionActive||!Ctx.IsAlive())
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Enemy : SessionInactiveOrDead");
		return Out;
	}

	// 체인/연출로 적 공격 금지 → 완전 정지
	if (Ctx.bEnemySuppressed)
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Enemy : SuppressedByChain");
		return Out;
	}

	// CC면 행동불가
	if (Ctx.IsCCBlocked())
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Enemy : CCBlocked");
		return Out;
	}

	// 그로기/라이징 상태 규약
	if (State == EEnemyCombatAIState::Groggy_Stunned)
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Enemy:GroggyStunnedNoAction");
		return Out;
	}

	if (State == EEnemyCombatAIState::Rising && !PresetAsset.Enemy.bRisingAttackAllowed)
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason =TEXT("Enemy : RisingNoAttack");
		return Out;
	}

	if (!CurrentTarget)
	{
		Out.Type = ECombatAIActionType::Wait;
		Out.DebugReason = TEXT("Enemy : NoTarget");
		return Out;
	}

	// 패턴 스킬 선택(내부 쿨/가중치 랜덤은 EnemyAIController에서 “상태”를 추가로 고려)
	// 여기서는 “가능한 스킬 있으면 1개”만 고르는 간단 버전
	for (const FEnemySkillPatternEntry &E : PresetAsset.Enemy.Pattern)
	{
		if (E.SkillId.IsNone()) continue;

		FGameplayTag FailReason;
		if (Ctx.Skill && Ctx.Skill->CanUseSkill(E.SkillId, FailReason))
		{
			Out.Type = ECombatAIActionType::UseSkill;
			Out.SkillId = E.SkillId;
			Out.Target = CurrentTarget;
			Out.Score = 500.f;
			Out.DebugReason = TEXT("Enemy : PatternSkill");
			return Out;
		}
	}

	Out.Type = ECombatAIActionType::BasicAttack;
	Out.Target = CurrentTarget;
	Out.Score = 200.f;
	Out.DebugReason = TEXT("Enemy : BasicAttackFallback");
	return Out;
}