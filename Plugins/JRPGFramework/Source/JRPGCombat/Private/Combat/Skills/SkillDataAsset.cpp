#include "Combat/Skills/SkillDataAsset.h"

bool USkillDataAsset::Validate(FJRPGReason& OutReason) const
{
	if (SkillId.IsNone())
	{
		OutReason = FJRPGReason::Make("Skill.InvalidId");
		return false;
	}
	
	if (Effects.Num == 0)
	{
		OutReason = FJRPGReason::Make("Skill.NoEffects");
		return false;
	}
	
	//최소 필드 체크(확장 가능)
	for (const FJRPGSkillEffectEntry& E : Effects)
	{
		switch (E.Kind)
		{
		case EJRPGSkillEffectKind::DealDamage:
		case EJRPGSkillEffectKind::Heal:
			if (E.Damage.Amount <= 0.f)
			{
				OutReason = FJRPGReason::Make("Skill.BadDamageAmount");
				return false;
			}
			break;

		case EJRPGSkillEffectKind::ApplyStatus:
			if (E.Status.StatusId.IsNone())
			{
				OutReason = FJRPGReason::Make("Skill.BadStatusId");
				return false;
			}
			break;

		case EJRPGSkillEffectKind::RequestMotion:
			if (E.Motion.Distance <= 0.f && E.Motion.ExecMode != ECombatMotionExecMode::Teleport && E.Motion.ExecMode != ECombatMotionExecMode::RootMotion)
			{
				OutReason = FJRPGReason::Make("Skill.BadMotionDistance");
				return false;
			}
			break;

		default:
			break;
		}
	}

	return true;
}