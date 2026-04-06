#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ExplorationViewModel.generated.h"

class UTexture2D;

// 퀘스트 UI 업데이트를 위한 델리게이트 (아이콘, 목표 텍스트)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuestUpdated, UTexture2D* /*QuestIcon*/, const FString& /*Objective*/);

UCLASS()
class JRPG_API UExplorationViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* World);
	void Deinitialize();

	FOnQuestUpdated OnQuestUpdated;

	// 통합 테스트를 위한 임시 퀘스트 데이터 푸시
	void LoadTempQuestData();

private:
	TWeakObjectPtr<UWorld> CachedWorld;
};