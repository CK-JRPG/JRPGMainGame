#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ExplorationViewModel.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FPartyChatMsg
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> SpeakerIcon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString Message = TEXT("");
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuestUpdated, UTexture2D* /*QuestIcon*/, const FString& /*Objective*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyChatReceived, const FPartyChatMsg&); // 채팅용
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRegionChanged, const FString&);

UCLASS()
class JRPG_API UExplorationViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* World);
	void Deinitialize();

	FOnQuestUpdated OnQuestUpdated;
	FOnPartyChatReceived OnPartyChatReceived;
	FOnRegionChanged OnRegionChanged;

	// [테스트용] 프레젠터 등에서 호출하여 UI가 잘 뜨는지 확인하기 위한 함수
	void PushTestPartyChat(UTexture2D* Icon, const FString& Text);
	void PushTestRegionName(const FString& RegionName);

	// 통합 테스트를 위한 임시 퀘스트 데이터 푸시
	void LoadTempQuestData();

private:
	TWeakObjectPtr<UWorld> CachedWorld;
};