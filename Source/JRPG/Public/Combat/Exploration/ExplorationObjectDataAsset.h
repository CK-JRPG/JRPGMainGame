// Source/JRPGCombat/Public/Combat/Exploration/ExplorationObjectDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "Combat/Exploration/ExplorationTypes.h"
#include "Combat/Exploration/ExplorationRewardTypes.h"
#include "ExplorationObjectDataAsset.generated.h"

// 문서: UExplorationObjectDataAsset 필드 고정 :contentReference[oaicite:22]{index=22}
UCLASS()
class JRPG_API UExplorationObjectDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 문서: ObjectId (GUID 또는 해시) :contentReference[oaicite:23]{index=23}
	UPROPERTY(EditAnywhere) FGuid ObjectId;

	UPROPERTY(EditAnywhere) EExplorationObjectType Type = EExplorationObjectType::Chest;
	UPROPERTY(EditAnywhere) EExplorationTriggerType TriggerType = EExplorationTriggerType::Interact;

	UPROPERTY(EditAnywhere) float InteractRange = 250.f;
	UPROPERTY(EditAnywhere) bool bRequiresLOS = false;

	UPROPERTY(EditAnywhere) FExplorationLockCondition LockCondition;

	// RewardTableId 또는 Entries 둘 중 하나(또는 둘 다) :contentReference[oaicite:24]{index=24}
	UPROPERTY(EditAnywhere) TObjectPtr<class UExplorationRewardTableAsset> RewardTable = nullptr;
	UPROPERTY(EditAnywhere) TArray<FRewardEntry> RewardEntries;

	UPROPERTY(EditAnywhere) EExplorationAcquisitionPolicy AcquisitionPolicy = EExplorationAcquisitionPolicy::OneTime;
	UPROPERTY(EditAnywhere) float RespawnTimeSec = 0.f;// Respawn일 때만 :contentReference[oaicite:25]{index=25}

	// UI
	UPROPERTY(EditAnywhere) FText UIName;
	UPROPERTY(EditAnywhere,Multiline) FText UIDescription;
	UPROPERTY(EditAnywhere) TObjectPtr<UTexture2D> Icon = nullptr;

	// 맵 표식 타입(맵 시스템 붙일 때 사용)
	UPROPERTY(EditAnywhere) FName MapMarkerType = NAME_None;

	// 확장 슬롯: 상점/특수 연동은 Flag로 처리해두면 안전
	UPROPERTY(EditAnywhere) FName ExtensionFlag = NAME_None;

	bool IsValidObject() const { return ObjectId.IsValid(); }
};