// Source/JRPGCombat/Private/Combat/AI/CombatAIPresetAsset.cpp
#include "Combat/AI/CombatAIPresetAsset.h"

const FCombatAIPartyTuning& UCombatAIPresetAsset::GetPartyTuning(ECombatPartyRole Role)const
{
	switch (Role)
	{
		case ECombatPartyRole::Defender: return Defender;
		case ECombatPartyRole::Attacker: return Attacker;
		default: return Supporter;
	}
}

FCombatAIWeights UCombatAIPresetAsset::GetEffectiveWeights(ECombatPartyRole Role,ECombatAIPreset Preset)const
{
	const FCombatAIPartyTuning &T = GetPartyTuning(Role);
	switch (Preset)
	{
		case ECombatAIPreset::Aggressive: return T.BaseWeights+T.AggressiveOffset;
		case ECombatAIPreset::Defensive: return T.BaseWeights+T.DefensiveOffset;
		default: return T.BaseWeights;
	}
}

float UCombatAIPresetAsset::GetHealThreshold(ECombatAIPreset Preset) const
{
	// Supporter tuning 값 사용 (Defender/Attacker도 “위기 시 힐”을 허용하려면 여기 규칙 확장)
	switch (Preset)
	{
		case ECombatAIPreset::Aggressive: return Supporter.HealThreshold_Aggressive;
		case ECombatAIPreset::Defensive: return Supporter.HealThreshold_Defensive;
		default: return Supporter.HealThreshold_Basic;
	}
}

float UCombatAIPresetAsset::GetReservedApHoldStrength(ECombatAIPreset Preset)const
{
	switch (Preset)
	{
		case ECombatAIPreset::Aggressive: return Supporter.ReservedApHoldStrength_Aggressive;
		case ECombatAIPreset::Defensive: return Supporter.ReservedApHoldStrength_Defensive;
		default: return Supporter.ReservedApHoldStrength_Basic;
	}
}