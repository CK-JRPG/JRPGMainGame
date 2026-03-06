// Source/JRPGCombat/Private/Combat/Exploration/ExplorationDiscoveryVolume.cpp
#include "Combat/Exploration/ExplorationDiscoveryVolume.h"

#include "Combat/Exploration/ExplorationSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AExplorationDiscoveryVolume::AExplorationDiscoveryVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("DiscoveryBox"));
	RootComponent = Box;
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AExplorationDiscoveryVolume::BeginPlay()
{
	Super::BeginPlay();

	Box->OnComponentBeginOverlap.AddDynamic(this, &AExplorationDiscoveryVolume::OnOverlapBegin);
}

void AExplorationDiscoveryVolume::OnOverlapBegin(UPrimitiveComponent* /*Overlapped*/, AActor* OtherActor,
                                                 UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
                                                 bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!OtherActor) return;

	if (UExplorationSubsystem* Explore = GetWorld()->GetSubsystem<UExplorationSubsystem>())
	{
		Explore->TryDiscover(DiscoveryId, OtherActor, OptionalDiscoveryReward);
	}
}
