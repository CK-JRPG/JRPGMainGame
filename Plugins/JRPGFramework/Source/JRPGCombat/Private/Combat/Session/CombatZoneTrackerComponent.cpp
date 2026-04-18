#include "Combat/Session/CombatZoneTrackerComponent.h"

#include "Combat/Session/CombatZoneActor.h"
#include "Combat/Movement/JRPGCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

UCombatZoneTrackerComponent::UCombatZoneTrackerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatZoneTrackerComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatZoneTracker] Owner must be ACharacter."));
		return;
	}

	UCapsuleComponent* Cap = Char->GetCapsuleComponent();
	if (!Cap)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatZoneTracker] Capsule missing."));
		return;
	}

	Cap->OnComponentBeginOverlap.AddDynamic(this, &UCombatZoneTrackerComponent::OnOwnerBeginOverlap);
	Cap->OnComponentEndOverlap.AddDynamic(this, &UCombatZoneTrackerComponent::OnOwnerEndOverlap);
}

void UCombatZoneTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CurrentZone.Reset();
	Super::EndPlay(EndPlayReason);
}

void UCombatZoneTrackerComponent::OnOwnerBeginOverlap(
	UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ACombatZoneActor* Zone = Cast<ACombatZoneActor>(OtherActor);
	if (!Zone) return;

	CurrentZone = Zone;
	ApplyZoneToMovement(Zone);
}

void UCombatZoneTrackerComponent::OnOwnerEndOverlap(
	UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACombatZoneActor* Zone = Cast<ACombatZoneActor>(OtherActor);
	if (!Zone) return;

	// 현재 존이 나간 존이면 해제
	if (CurrentZone.Get() == Zone)
	{
		CurrentZone.Reset();
		ApplyZoneToMovement(nullptr);
	}
}

void UCombatZoneTrackerComponent::ApplyZoneToMovement(ACombatZoneActor* Zone)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char) return;

	UJRPGCharacterMovementComponent* Move = Cast<UJRPGCharacterMovementComponent>(Char->GetCharacterMovement());
	if (!Move) return;

	Move->SetCurrentCombatZone(Zone);
}
