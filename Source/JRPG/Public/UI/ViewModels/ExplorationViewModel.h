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
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyChatReceived, const FPartyChatMsg&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRegionChanged, const FString&);

UENUM(BlueprintType)
enum class EEnemyIndicatorState : uint8
{
	Nearby = 0, // 노란색 (근처에 있음)
	Chasing = 1 // 빨간색 (발견/추격)
};

// 적 감지 데이터
USTRUCT(BlueprintType)
struct FEnemyIndicatorData
{
	GENERATED_BODY()
	UPROPERTY() FGuid EnemyId;
	UPROPERTY() EEnemyIndicatorState State;
	UPROPERTY() FVector WorldLocation;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInteractionChanged, bool /*bCanInteract*/, const FString& /*ActionText*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDialogueChanged, bool /*bIsActive*/, const FString& /*Speaker*/, const FString& /*Text*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyIndicatorsUpdated, const TArray<FEnemyIndicatorData>&);

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

	FOnInteractionChanged OnInteractionChanged;
	FOnDialogueChanged OnDialogueChanged;
	FOnEnemyIndicatorsUpdated OnEnemyIndicatorsUpdated;

	// [테스트용] 프레젠터 등에서 호출하여 UI 확인
	void PushTestPartyChat(UTexture2D* Icon, const FString& Text);
	void PushTestRegionName(const FString& RegionName);
	void PushTestInteraction(bool bShow, const FString& Text = TEXT(""));
	void PushTestDialogue(bool bShow, const FString& Speaker = TEXT(""), const FString& Text = TEXT(""));

	// 통합 테스트를 위한 임시 퀘스트 데이터
	void LoadTempQuestData();

private:
	TWeakObjectPtr<UWorld> CachedWorld;
};