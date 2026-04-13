#include "Game/HubSubsystem.h"
#include "Game/InteractableInterface.h"

// ==== 허브 등록 ====
void UHubSubsystem::RegisterHub(AActor* HubActor)
{
	if (!HubActor) return;
	RegisteredHubs.AddUnique(HubActor);
	UE_LOG(LogTemp, Log, TEXT("HubSubsystem : 허브 등록 - %s"), *GetNameSafe(HubActor));
}

void UHubSubsystem::UnregisterHub(AActor* HubActor)
{
	if (!HubActor) return;
	RegisteredHubs.Remove(HubActor);

	// 해제되는 허브가 FocusedHub이면 클리어
	if (FocusedHub.Get() == HubActor)
	{
		FocusedHub = nullptr;
	}

	// 해제되는 허브가 LastVisitedHub이면 클리어
	if (LastVisitedHub.Get() == HubActor)
	{
		LastVisitedHub = nullptr;
		LastVisitedLocation = FVector::ZeroVector;
	}

	UE_LOG(LogTemp, Log, TEXT("HubSubsystem : 허브 해제 - %s"), *GetNameSafe(HubActor));
}

// ==== 포커스 허브 ====
void UHubSubsystem::SetFocusedHub(AActor* HubActor)
{
	FocusedHub = HubActor;
	UE_LOG(LogTemp, Log, TEXT("HubSubsystem : 포커스 허브 설정 - %s"), *GetNameSafe(HubActor));

	// UI를 켜고 텍스트 전달
	if (IInteractableInterface* InteractableTarget = Cast<IInteractableInterface>(HubActor))
	{
		OnInteractableTargetChanged.Broadcast(true, InteractableTarget->GetInteractText());
	}
}

void UHubSubsystem::ClearFocusedHub(AActor* HubActor)
{
	// 현재 포커스 허브와 동일한 경우만 클리어 (다른 허브가 이미 포커스된 경우 무시)
	if (FocusedHub.Get() == HubActor)
	{
		FocusedHub = nullptr;
		UE_LOG(LogTemp, Log, TEXT("HubSubsystem : 포커스 허브 해제 - %s"), *GetNameSafe(HubActor));

		// UI 끄기
		OnInteractableTargetChanged.Broadcast(false, TEXT(""));
	}
}

AActor* UHubSubsystem::GetFocusedHub() const
{
	return FocusedHub.Get();
}

// ==== 마지막 방문 허브 ====
void UHubSubsystem::VisitHub(AActor* HubActor, const FVector& PlayerLocation)
{
	if (!HubActor) return;
	LastVisitedHub = HubActor;
	LastVisitedLocation = PlayerLocation;
	UE_LOG(LogTemp, Log, TEXT("HubSubsystem : 마지막 방문 허브 등록 - %s (플레이어 위치: %s)"), *GetNameSafe(HubActor), *PlayerLocation.ToString());
}

bool UHubSubsystem::HasLastVisitedHub() const
{
	return LastVisitedHub.IsValid();
}

// ==== 리스폰 위치 ====
FVector UHubSubsystem::GetRespawnLocation(const FVector& FallbackOrigin) const
{
	// 마지막 방문 허브가 유효하면 -> E키 상호작용 시점의 플레이어 위치 반환
	if (LastVisitedHub.IsValid())
	{
		return LastVisitedLocation;
	}

	// 마지막 방문 허브가 없으면 가장 가까운 허브로 폴백
	bool bFoundNearestHub = false;
	const FVector NearestHubLocation = FindNearestHubLocation(FallbackOrigin, bFoundNearestHub);
	if (bFoundNearestHub)
	{
		return NearestHubLocation + FVector(0.f, 0.f, 100.f);
	}

	UE_LOG(LogTemp, Warning, TEXT("HubSubsystem : 등록된 허브가 없어 FallbackOrigin 사용 - %s"), *FallbackOrigin.ToString());
	return FallbackOrigin;
}

FVector UHubSubsystem::FindNearestHubLocation(const FVector& Origin, bool& bOutFoundHub) const
{
	FVector BestLocation = FVector::ZeroVector;
	float BestDistSq = TNumericLimits<float>::Max();
	bool bFound = false;

	for (const TWeakObjectPtr<AActor>& WeakHub : RegisteredHubs)
	{
		if (AActor* Hub = WeakHub.Get())
		{
			const float DistSq = FVector::DistSquared(Origin, Hub->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestLocation = Hub->GetActorLocation();
				bFound = true;
			}
		}
	}

	bOutFoundHub = bFound;
	return BestLocation;
}