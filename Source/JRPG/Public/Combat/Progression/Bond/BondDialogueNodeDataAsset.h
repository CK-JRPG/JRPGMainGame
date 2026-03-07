// Source/JRPGCombat/Public/Combat/Progression/Bond/BondDialogueNodeDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Progression/Bond/BondTypes.h"
#include "BondDialogueNodeDataAsset.generated.h"

// 문서 7.5 (SSOT) :contentReference[oaicite:21]{index=21}
UENUM()
enum class EBondDialoguePriorityRule : uint8
{
	RegionFirst,
	CombatRecapFirst,
	Neutral
};

UCLASS()
class JRPG_API UBondDialogueNodeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FName NodeId = NAME_None;

	// Participants (Pair/Trio) :contentReference[oaicite:22]{index=22}
	UPROPERTY(EditAnywhere)
	TArray<FName> Participants;

	UPROPERTY(EditAnywhere)
	int32 MinBondLevel = 1;
	UPROPERTY(EditAnywhere)
	TArray<FName> RequiredFlags;

	UPROPERTY(EditAnywhere)
	EBondDialoguePriorityRule PriorityRule = EBondDialoguePriorityRule::Neutral;

	// RewardBP (기본 규칙 위에 추가 보너스) :contentReference[oaicite:23]{index=23}
	UPROPERTY(EditAnywhere)
	int32 RewardBP = 0;

	// 대사/연출 리소스 참조(프로젝트에 맞춰 경로/에셋으로 교체)
	UPROPERTY(EditAnywhere)
	FSoftObjectPath DialogueContentRef;

	bool IsValidNode() const
	{
		if (NodeId.IsNone()) return false;
		if (Participants.Num() != 2 && Participants.Num() != 3) return false;

		TSet<FName> S;
		for (const FName& P : Participants)
		{
			if (P.IsNone()) return false;
			S.Add(P);
		}
		return (S.Num() == Participants.Num());
	}
};
