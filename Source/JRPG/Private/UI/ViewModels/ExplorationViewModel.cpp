#include "UI/ViewModels/ExplorationViewModel.h"

void UExplorationViewModel::Initialize(UWorld* World)
{
	CachedWorld = World;
	// 추후 UQuestSubsystem 등의 델리게이트를 여기서 구독합니다.
}

void UExplorationViewModel::Deinitialize()
{
	// 델리게이트 구독 해제
}

void UExplorationViewModel::LoadTempQuestData()
{
	// 기획 파트 회의 전까지 사용할 임시 테스트용 데이터
	FString TempObjective = TEXT("잃어버린 고대 유물 찾기 (0/1)\n마을 밖 숲으로 이동하세요.");

	// 임시 아이콘 (nullptr로 보내면 View에서 기본 아이콘 사용)
	UTexture2D* TempIcon = nullptr;

	OnQuestUpdated.Broadcast(TempIcon, TempObjective);
}