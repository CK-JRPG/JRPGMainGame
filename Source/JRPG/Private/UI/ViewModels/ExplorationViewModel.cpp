#include "UI/ViewModels/ExplorationViewModel.h"

void UExplorationViewModel::Initialize(UWorld* World)
{
	CachedWorld = World;
}

void UExplorationViewModel::Deinitialize()
{
}

void UExplorationViewModel::PushTestPartyChat(UTexture2D* Icon, const FString& Text)
{
	FPartyChatMsg NewMsg;
	NewMsg.SpeakerIcon = Icon;
	NewMsg.Message = Text;
	OnPartyChatReceived.Broadcast(NewMsg);
}

void UExplorationViewModel::PushTestRegionName(const FString& RegionName)
{
	OnRegionChanged.Broadcast(RegionName);
}

void UExplorationViewModel::LoadTempQuestData()
{
	// 테스트용 데이터
	FString TempObjective = TEXT("마을 밖 숲으로 이동하세요.");

	UTexture2D* TempIcon = nullptr;

	OnQuestUpdated.Broadcast(TempIcon, TempObjective);
}