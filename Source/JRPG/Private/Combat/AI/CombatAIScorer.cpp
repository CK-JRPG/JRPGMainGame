// Source/JRPGCombat/Private/Combat/AI/CombatAIScorer.cpp

#include "Combat/AI/CombatAIScorer.h"
#include "Combat/Skills/SkillComponent.h"

void UCombatAIScorer::Initialize(FGetSkillAIMetaDelegate InMetaResolver)
{
	MetaResolver = InMetaResolver;
}

float UCombatAIScorer::ScoreAction(const UCombatAIContext &Ctx, const FJRPGCombatAIAction &Action) const
{
	if (!Ctx.Owner.IsValid())
		return -FLT_MAX;

	// Chain 시퀀스에서는 일반 AI 선택을 하지 않는다.
	if (Ctx.bInChainSequence)
		return -FLT_MAX;

	if (Ctx.bSelfIsDead)
		return -FLT_MAX;

	// 기본: Wait는 매우 낮게
	if (Action.Type == EJRPGCombatAIActionType::Wait)
		return 0.05f;

	// BasicAttack은 항상 후보로 남김
	if (Action.Type == EJRPGCombatAIActionType::BasicAttack)
	{
		// “기본 공격은 오토, 스킬은 AP 기반” 전제 :contentReference[oaicite:21]{index=21}
		return 0.5f;
	}

	if (Action.Type != EJRPGCombatAIActionType::UseSkill||Action.SkillId.IsNone())
		return 0.f;

	FSkillAIMeta Meta;
	if (MetaResolver.IsBound())
	{
		// Resolver가 false면 Meta는 기본값(=모름)으로 처리
		MetaResolver.Execute(Ctx.SkillComp.Get(), Action.SkillId, Meta);
	}

	float S =0.f;
	S += ScoreRoleLogic(Ctx, Action, Meta);
	S += ScoreSPOpportunity(Ctx, Action, Meta);

	// SP 스팸 방지(동일 이벤트 반복 억제)을 AI 쪽에서도 살짝 반영:
	// 완전한 억제는 SP 시스템이 하지만, AI가 같은 행동만 연타하는 걸 줄여준다. :contentReference[oaicite:22]{index=22}
	const float SameEventCooldown = Ctx.SPSettings.SameEventCooldownSec;
	if (SameEventCooldown > 0.f)
	{
		// 여기서는 대충 SoftCap로만 처리
		S = SoftCapPenalty(S,10.f);
	}

	return S;
}

float UCombatAIScorer::ScoreRoleLogic(const UCombatAIContext &Ctx,const FJRPGCombatAIAction &A,const FSkillAIMeta &Meta)const
{
	const UCombatAIPresetAsset *Preset = Ctx.PresetAsset.Get();
	const FPartyAIWeights W = Preset ? Preset->Weights : FPartyAIWeights();
	const FPartyAIThresholds T = Preset ? Preset->Thresholds : FPartyAIThresholds();

	float Score = 0.f;

	// ---- Supporter(서포터): HP<30% 힐 > 정화 > 버프/디버프 > 공격 :contentReference[oaicite:23]{index=23}
	if (Ctx.Role == EJRPGPartyRole::Supporter)
	{
		const float HealThr = T.SupporterHealCriticalHp01;

		if (Meta.bIsHeal && Ctx.bAnyAllyCritical && Ctx.AllyCriticalTarget.IsValid())
			Score += W.HealCritical;

		if (Meta.bIsCleanse && Ctx.bAnyAllyHasCC && Ctx.AllyCC_Target.IsValid())
			Score += W.Cleanse;

		if (Meta.bIsBuff)
			Score += W.BuffUptime;

		if (Meta.bIsDebuff || Meta.bIsBreak)
			Score += 0.5f;// 여유 시 공격/브레이크 보조 :contentReference[oaicite:24]{index=24}

		// 기본 힐 임계는 프리셋에서 바뀔 수 있음(문서 예시) :contentReference[oaicite:25]{index=25}
		(void) HealThr;
	}

	// ---- Defender(탱커): Threat 유지 > 보호 > 브레이크 보조 > 공격 :contentReference[oaicite:26]{index=26}
	else if (Ctx.Role == EJRPGPartyRole::Defender)
	{
		if (Meta.bIsTaunt)
			Score += W.ThreatHold;

		if (Ctx.bAnyAllyCritical && (Meta.bIsTaunt || Meta.bIsBuff))
			Score += W.ProtectAlly;// 위험 대상 발생 시 보호/도발 우선 :contentReference[oaicite:27]{index=27}

		if (Meta.bIsBreak || Meta.bIsDebuff)
			Score += 0.5f;// 그로기 유도 보조 :contentReference[oaicite:28]{index=28}

		if (Meta.bIsHighDps)
			Score += 0.25f;
	}

	// ---- Attacker(딜러): 브레이크/딜 > 디버프 유지 > 공격 :contentReference[oaicite:29]{index=29}
	else
	{
		if (Meta.bIsBreak)
			Score += W.BreakBuild;

		if (Meta.bIsDebuff)
			Score += 1.0f;

		if (Meta.bIsHighDps)
			Score += W.HighDps;

		// 브레이크 임계 근접 시 브레이크 스킬 우선 :contentReference[oaicite:30]{index=30}
		if (Ctx.TargetBreakRatio01 >= T.BreakNearThreshold01 && Meta.bIsBreak)
			Score += 2.0f;
	}

	return Score;
}

float UCombatAIScorer::ScoreSPOpportunity(const UCombatAIContext &Ctx,const FJRPGCombatAIAction &A,const FSkillAIMeta &Meta) const
{
	const UCombatAIPresetAsset *Preset = Ctx.PresetAsset.Get();
	const FPartyAIWeights W = Preset ? Preset->Weights : FPartyAIWeights();

	// SP는 롤에 맞는 행동 보상 :contentReference[oaicite:31]{index=31}
	// 이벤트 예시는:
	// - Defender: AggroHold/Rescue/Protect
	// - Attacker: BreakContribution/StunTrigger/DamageWindow
	// - Supporter: CriticalHeal/Cleanse/BuffUptime :contentReference[oaicite:32]{index=32}
	float Score = 0.f;

	if (Ctx.Role == EJRPGPartyRole::Defender)
	{
		if (Meta.bIsTaunt) 
			Score += 1.5f;
		
		if (Ctx.bAnyAllyCritical && (Meta.bIsTaunt || Meta.bIsBuff)) 
			Score += 2.0f;
	}
	else if (Ctx.Role == EJRPGPartyRole::Attacker)
	{
		if (Meta.bIsBreak) 
			Score += 2.0f;
		
		if (Meta.bIsHighDps) 
			Score += 1.0f;
	}
	else
	{
		if (Meta.bIsHeal && Ctx.bAnyAllyCritical) 
			Score += 2.5f;
		
		if (Meta.bIsCleanse && Ctx.bAnyAllyHasCC) 
			Score += 2.0f;
		
		if (Meta.bIsBuff) 
			Score += 1.0f;
	}

	Score *= FMath::Max(0.1f,W.SPBonusMultiplier);
	return Score;
}

float UCombatAIScorer::SoftCapPenalty(float Value, float SoftCap)
{
	if (SoftCap <= 0.f) return Value;
	if (Value <= SoftCap) return Value;
	return SoftCap + (Value-SoftCap) * 0.25f;
}