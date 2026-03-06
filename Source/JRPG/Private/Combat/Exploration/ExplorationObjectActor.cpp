#include "Combat/Exploration/ExplorationObjectActor.h"

#include "Combat/Exploration/ExplorationSubsystem.h"
#include "Combat/Exploration/ExplorationObjectDataAsset.h"

#include "Engine/World.h"

AExplorationObjectActor::AExplorationObjectActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AExplorationObjectActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* W = GetWorld())
	{
		if (UExplorationSubsystem* Explore = W->GetSubsystem<UExplorationSubsystem>())
			Explore->RegisterObject(this);
	}
}

void AExplorationObjectActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		if (UExplorationSubsystem* Explore = W->GetSubsystem<UExplorationSubsystem>())
			Explore->UnregisterObject(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AExplorationObjectActor::SetExplorationActive(bool bActive)
{
	bIsActive = bActive;
	SetActorHiddenInGame(!bIsActive);
	SetActorEnableCollision(bIsActive);
}
