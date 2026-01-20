#include "BehaviorPresetDataAsset.h"

float UBehaviorPresetDataAsset::GetScore(const FGameplayTag& Tag) const
{
	for (const FTagScore& TS : TagScores)
		if (TS.Tag == Tag) return TS.Score;
	return 0.f;
}