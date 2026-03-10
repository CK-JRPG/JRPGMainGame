#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StatusTypes.h"
#include "StatusDataAsset.generated.h"

USTRUCT()
struct FStatusDef
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) FName StatusId = NAME_None;
	UPROPERTY(EditDefaultsOnly) float DefaultDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly) bool bIsCC = false;
	UPROPERTY(EditDefaultsOnly) EStatusStackPolicy StackPolicy = EStatusStackPolicy::RefreshDuration;
	UPROPERTY(EditDefaultsOnly) int32 MaxStacks = 1;
};

UCLASS()
class JRPGCOMBAT_API UStatusDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly) TArray<FStatusDef> Statuses;

	const FStatusDef* FindDef(FName StatusId) const
	{
		return Statuses.FindByPredicate([&](const FStatusDef& D){ return D.StatusId == StatusId; });
	}
};
