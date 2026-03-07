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
		GetWorld()->GetTimerManager().SetTimer(ScanTimer, this, &UWorldInteractComponent::ScanOnce, ScanIntervalSec,
		                                       true);
}

void UWorldInteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(ScanTimer);

	Super::EndPlay(EndPlayReason);
}

UExplorationSubsystem* UWorldInteractComponent::GetExplore() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UExplorationSubsystem>() : nullptr;
}

bool UWorldInteractComponent::HasLOSToActor(const AActor* Target) const
{
	if (!GetOwner() || !Target || !GetWorld()) return false;

	FHitResult Hit;
	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Target->GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldInteractLOS), false, GetOwner());
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, LOSTraceChannel, Params);

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

		// 프롬프트는 Interact 트리거만
		if (Data->TriggerType != EExplorationTriggerType::Interact)
			continue;

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

	if (BestId != CurrentObjectId)
	{
		if (CurrentObjectId.IsValid())
			Explore->NotifyPromptChanged(CurrentObjectId, false);

		CurrentObjectId = BestId;

		if (CurrentObjectId.IsValid())
			Explore->NotifyPromptChanged(CurrentObjectId, true);
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
