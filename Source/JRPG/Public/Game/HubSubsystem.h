#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HubSubsystem.generated.h"

/**
 * 허브 등록/해제, 최근 방문 허브 위치 관리
 */
UCLASS()
class JRPG_API UHubSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// 허브 등록
	void RegisterHub(AActor* Hub);
	void UnregisterHub(AActor* Hub);
	
	// 플레이어가 현재 범위 안에 있는 허브 추적 (상호작용 대상)
	void SetFocusedHub(AActor* Hub);
	void ClearFocusedHub(AActor* Hub);
	AActor* GetFocusedHub() const;
	
	// 마지막 방문 허브
	void VisitHub(AActor* HubActor);
	bool HasLastVisitedHub() const;

	// 리스폰 위치
	// 마지막 방문 허브가 있으면 해당 위치, 없으면 가장 가까운 허브 위치 반환
	FVector GetRespawnLocation(const FVector& FallbackOrigin) const;
	
private:
	// 가장 가까운 허브 위치 검색 (폴백용)
	FVector FindNearestHubLocation(const FVector& Origin) const;
	
	// 나중에 전체 허브 목록이 필요할거 같아서 제작했음.
	// 예) 패스트 트래블 UI, 미니맵 허브 표시
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> RegisteredHubs;
	
	UPROPERTY() TWeakObjectPtr<AActor> FocusedHub;
	UPROPERTY() TWeakObjectPtr<AActor> LastVisitedHub;
};
