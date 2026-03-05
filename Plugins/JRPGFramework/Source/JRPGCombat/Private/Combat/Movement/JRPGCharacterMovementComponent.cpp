#include "Combat/Movement/JRPGCharacterMovementComponent.h"

#include "Combat/Session/CombatZoneActor.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

void UJRPGCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (!bZoneClampEnabled) return;

	ACombatZoneActor* Zone = CurrentCombatZone.Get();
	if (!Zone) return;

	ACharacter* Char = CharacterOwner;
	if (!Char) return;

	UCapsuleComponent* Cap = Char->GetCapsuleComponent();
	if (!Cap) return;

	const float Radius = Cap->GetScaledCapsuleRadius();
	const float HalfH  = Cap->GetScaledCapsuleHalfHeight();

	const FVector Current = Char->GetActorLocation();
	const FVector Clamped = Zone->ClampCharacterLocation(Current, Radius, HalfH);

	if (!Clamped.Equals(Current, 0.1f))
	{
		if (bTeleportClamp)
		{
			// 가장 확실한 보정
			Char->SetActorLocation(Clamped, false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			// 스윕 보정(벽/충돌 고려) - 필요 시 사용
			FHitResult Hit;
			Char->SetActorLocation(Clamped, true, &Hit, ETeleportType::None);
		}
	}
}