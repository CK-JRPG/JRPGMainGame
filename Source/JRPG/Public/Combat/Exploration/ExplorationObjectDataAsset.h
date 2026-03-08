#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Exploration/ExplorationTypes.h"
#include "Combat/Exploration/ExplorationRewardTypes.h"
#include "ExplorationObjectDataAsset.generated.h"

UCLASS()
class JRPG_API UExplorationObjectDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 고유 ID(저장 키)
	UPROPERTY(EditAnywhere)
	FGuid ObjectId;

	UPROPERTY(EditAnywhere)
	EExplorationObjectType Type = EExplorationObjectType::Chest;
	
	UPROPERTY(EditAnywhere)
	EExplorationTriggerType TriggerType = EExplorationTriggerType::Interact;

	UPROPERTY(EditAnywhere)
	float InteractRange = 250.f;
	
	UPROPERTY(EditAnywhere)
	bool bRequiresLOS = false;

	UPROPERTY(EditAnywhere)
	FExplorationLockCondition LockCondition;

	// RewardTable + DirectEntries(혼용 가능)
	UPROPERTY(EditAnywhere)
	TObjectPtr<UExplorationRewardTableAsset> RewardTable = nullptr;
	
	UPROPERTY(EditAnywhere)
	TArray<FRewardEntry> RewardEntries;

	UPROPERTY(EditAnywhere)
	EExplorationAcquisitionPolicy AcquisitionPolicy = EExplorationAcquisitionPolicy::OneTime;
	
	UPROPERTY(EditAnywhere)
	float RespawnTimeSec = 0.f;

	// UI/Map 확장
	UPROPERTY(EditAnywhere)
	FText UIName;
	
	UPROPERTY(EditAnywhere, Multiline)
	FText UIDescription;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UTexture2D> Icon = nullptr;
	
	UPROPERTY(EditAnywhere)
	FName MapMarkerType = NAME_None;

	// 상점/지역 해금 등은 Flag로 연결(상점 시스템에서 HasFlag로 소비)
	UPROPERTY(EditAnywhere)
	FName ExtensionFlag = NAME_None;

	bool IsValidObject() const { return ObjectId.IsValid(); }
};
