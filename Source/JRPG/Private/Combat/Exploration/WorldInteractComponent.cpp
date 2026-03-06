// Source/JRPGCombat/Private/Combat/Exploration/WorldInteractComponent.cpp
#include "Combat/Exploration/WorldInteractComponent.h"

#include "Combat/Exploration/ExplorationSubsystem.h"
#include "Combat/Exploration/ExplorationObjectActor.h"
#include "Combat/Exploration/ExplorationObjectDataAsset.h"

#include "Engine/World.h"
#include "TimerManager.h"

UWorldInteractComponent::UWorldInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWorldInteractComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(ScanTimer, this, &UWorldInteractComponent::ScanOnce, ScanIntervalSec,
		                                       true);
	}
}

void UWorldInteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ScanTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UExplorationSubsystem* UWorldInteractComponent::GetExplore() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UExplorationSubsystem>() : nullptr;
}

bool UWorldInteractComponent::HasLOSToActor(const AActor* Target) const
{
	if (!GetOwner() || !Target) return false;

	FHitResult Hit;
	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Target->GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldInteractLOS), false, GetOwner());
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, LOSTraceChannel, Params);

	// 막힌 게 없거나, 첫 히트가 타겟이면 LOS OK
	if (!bHit) return true;
	return Hit.GetActor() == Target;
}

void UWorldInteractComponent::ScanOnce()
{
	UExplorationSubsystem* Explore = GetExplore();
	if (!Explore || !GetOwner()) return;

	TArray<TWeakObjectPtr<AExplorationObjectActor>> Objs;
	Explore->GetAllRegisteredObjects(Objs);

	const FVector MyPos = GetOwner()->GetActorLocation();

	FGuid BestId;
	float BestDistSq = FLT_MAX;

	for (const TWeakObjectPtr<AExplorationObjectActor>& W : Objs)
	{
		AExplorationObjectActor* Obj = W.Get();
		if (!Obj) continue;

		const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
		if (!Data || !Data->IsValidObject()) continue;

		// Interact 트리거만 프롬프트 대상(Discovery는 볼륨/자동)
		if (Data->TriggerType != EExplorationTriggerType::Interact)
			continue;

		// 활성 상태
		if (Explore->GetObjectState(Data->ObjectId) != EExplorationObjectState::Active)
			continue;

		const float Range = FMath::Max(0.f, Data->InteractRange);
		const float DistSq = FVector::DistSquared(MyPos, Obj->GetActorLocation());
		if (DistSq > Range * Range) continue;

		if (Data->bRequiresLOS && !HasLOSToActor(Obj))
			continue;

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestId = Data->ObjectId;
		}
	}

	// 프롬프트 변화 이벤트: OnInteractPromptChanged(ObjectId, bVisible) :contentReference[oaicite:37]{index=37}
	if (BestId != CurrentObjectId)
	{
		// 이전 프롬프트 끄기
		if (CurrentObjectId.IsValid())
		{
			Explore->NotifyPromptChanged(CurrentObjectId, false);
		}

		CurrentObjectId = BestId;

		// 새 프롬프트 켜기
		if (CurrentObjectId.IsValid())
		{
			Explore->NotifyPromptChanged(CurrentObjectId, true);
		}
	}
}

void UWorldInteractComponent::TryInteractInput()
{
	UExplorationSubsystem* Explore = GetExplore();
	if (!Explore || !GetOwner()) return;

	if (!CurrentObjectId.IsValid())
		return;

	Explore->TryInteract(GetOwner(), CurrentObjectId);
}
