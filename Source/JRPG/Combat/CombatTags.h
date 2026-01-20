#pragma once
#include "NativeGameplayTags.h"

namespace CombatTags
{
	// ============= 탱커 =============
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Taunt);       // 단일 대상 도발
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_AoeTaunt);    // 광역 도발
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Protect);     // 아군 보호
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_ShieldAlly);  // 아군에게 보호막 주기
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_MitSelf);     // 자신의 피해 감소
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_CC);          // 군중 제어 - Crowd Controll
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_BreakHelp);   // 브레이크 게이지 지원

	// ============= 딜러 =============
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_DpsHigh);     // 화력 좋은 딜 스킬
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_DpsLow);      // 저화력 딜 스킬
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Break);       // 브레이크 게이지 공격
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_ThreatDown);  // 어그로 탱커한테 넘기는 건가

	// ============= 힐러 =============
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_HealSingle);  // 단일 대상 힐
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_HealAoE);     // 광역 힐
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Cleanse);     // 디버프 제거
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_MitAlly);     // 아군 피해 감소
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Buff);        // 아군 버프 주기
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Debuff);      // 적 디버프 주기
}
