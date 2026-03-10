#pragma once

#include "GameplayTagContainer.h"

/**
 * GameplayTags는 ini(예: DefaultGameplayTags.ini)에 등록하는 것을 전제로 한다.
 * 여기서는 코드에서 안전하게 접근할 수 있는 "접근자"만 제공한다.
 */
struct FCombatTags
{
	// Groggy / Break
	static FGameplayTag Immune_Break();// "Immune.Break"
	static FGameplayTag Debuff_BreakVuln();// "Debuff.BreakVuln"
	static FGameplayTag Debuff_BreakResistDown();// "Debuff.BreakResistDown"

	// CC / State
	static FGameplayTag CC_Stun();// "CC.Stun"
	static FGameplayTag State_Rising();// "State.Rising"

	// 디버그/이벤트 이유 태그 예시
	static FGameplayTag Reason_BreakReached();// "Reason.BreakReached"
	static FGameplayTag Reason_StunEnded();// "Reason.StunEnded"
	static FGameplayTag Reason_RisingEnded();// "Reason.RisingEnded"
};
