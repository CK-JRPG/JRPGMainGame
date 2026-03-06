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

void AExplorationDiscoveryVolume::OnOverlapBegin(UPrimitiveComponent*/*Overlapped*/, AActor* OtherActor,
                                                 UPrimitiveComponent*/*OtherComp*/, int32/*OtherBodyIndex*/,
                                                 bool/*bFromSweep*/, const FHitResult&/*SweepResult*/)
{
	if (!OtherActor) return;

	if (UWorld* W = GetWorld())
	{
		if (UExplorationSubsystem* Explore = W->GetSubsystem<UExplorationSubsystem>())
		{
			const FExplorationOp R = Explore->TryDiscover(DiscoveryId, OtherActor, OptionalDiscoveryReward);

			// 한번 발견되면 볼륨은 비활성화(중복 호출 방지)
			if (R.bOk || R.ReasonTag == "Reject.DuplicateDiscovery")
			{
				SetActorEnableCollision(false);
				SetActorHiddenInGame(true);
			}
		}
	}
}
