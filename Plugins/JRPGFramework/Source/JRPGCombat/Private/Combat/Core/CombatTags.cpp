#include"Combat/Core/CombatTags.h"
#include"GameplayTagsManager.h"

static FGameplayTag ReqTag(const TCHAR*Name)
{
	const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(Name), /*ErrorIfNotFound*/false);
	ensureMsgf(Tag.IsValid(), TEXT("GameplayTag not found: %s (등록 필요)"), Name);
	return Tag;
}

FGameplayTag FCombatTags::Immune_Break()           { static FGameplayTag T = ReqTag(TEXT("Immune.Break")); return T; }
FGameplayTag FCombatTags::Debuff_BreakVuln()       { static FGameplayTag T = ReqTag(TEXT("Debuff.BreakVuln")); return T; }
FGameplayTag FCombatTags::Debuff_BreakResistDown() { static FGameplayTag T = ReqTag(TEXT("Debuff.BreakResistDown")); return T; }
FGameplayTag FCombatTags::CC_Stun()                { static FGameplayTag T = ReqTag(TEXT("CC.Stun")); return T; }
FGameplayTag FCombatTags::State_Rising()           { static FGameplayTag T = ReqTag(TEXT("State.Rising")); return T; }

FGameplayTag FCombatTags::Reason_BreakReached()    { static FGameplayTag T = ReqTag(TEXT("Reason.BreakReached")); return T; }
FGameplayTag FCombatTags::Reason_StunEnded()       { static FGameplayTag T = ReqTag(TEXT("Reason.StunEnded")); return T; }
FGameplayTag FCombatTags::Reason_RisingEnded()     { static FGameplayTag T = ReqTag(TEXT("Reason.RisingEnded")); return T; }