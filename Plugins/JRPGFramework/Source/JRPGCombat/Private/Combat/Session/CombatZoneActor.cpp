#include "Combat/Session/CombatZoneActor.h"

#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"

ACombatZoneActor::ACombatZoneActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ZoneBounds = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneBounds"));
	SetRootComponent(ZoneBounds);

	// 존 추적은 파티/적 커스텀 채널 모두를 받아야 한다.
	ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneBounds->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	ZoneBounds->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
	ZoneBounds->SetGenerateOverlapEvents(true);

	ZoneBounds->SetSphereRadius(1200.0f);
	
	bDrawDebug = false;
}

FVector ACombatZoneActor::ClampCharacterLocation(const FVector& WorldLocation, float CapsuleRadius, float CapsuleHalfHeight) const
{
	if (!ZoneBounds) return WorldLocation;

	const FTransform T = ZoneBounds->GetComponentTransform();
	const FVector Scale3D = T.GetScale3D().GetAbs();
	const float Radius = ZoneBounds->GetUnscaledSphereRadius() * FMath::Max(Scale3D.X, Scale3D.Y);
	const float HalfHeight = ZoneHalfHeight * Scale3D.Z;

	// 월드 -> 로컬
	FVector Local = T.InverseTransformPosition(WorldLocation);

	const float AllowedRadius = FMath::Max(0.0f, Radius - CapsuleRadius);
	FVector2D LocalXY(Local.X, Local.Y);

	const float Dist2D = LocalXY.Size();
	if (Dist2D > AllowedRadius && Dist2D > KINDA_SMALL_NUMBER)
	{
		LocalXY *= (AllowedRadius / Dist2D);
		Local.X = LocalXY.X;
		Local.Y = LocalXY.Y;
	}

	const float MinZ = -HalfHeight + CapsuleHalfHeight;
	const float MaxZ = HalfHeight - CapsuleHalfHeight;

	Local.Z = (MinZ <= MaxZ) ? FMath::Clamp(Local.Z, MinZ, MaxZ) : 0.0f;

	// 로컬 -> 월드
	return T.TransformPosition(Local);
}

bool ACombatZoneActor::IsCharacterInside(const FVector& WorldLocation, float CapsuleRadius, float CapsuleHalfHeight) const
{
	if (!ZoneBounds) return false;

	const FTransform T = ZoneBounds->GetComponentTransform();
	const FVector Scale3D = T.GetScale3D().GetAbs();
	const float Radius = ZoneBounds->GetUnscaledSphereRadius() * FMath::Max(Scale3D.X, Scale3D.Y);
	const float HalfHeight = ZoneHalfHeight * Scale3D.Z;

	const FVector Local = T.InverseTransformPosition(WorldLocation);

	const float AllowedRadius = FMath::Max(0.0f, Radius - CapsuleRadius);
	const float Dist2D = FVector2D(Local.X, Local.Y).Size();

	const float MinZ = -HalfHeight + CapsuleHalfHeight;
	const float MaxZ = HalfHeight - CapsuleHalfHeight;
	const bool bInsideZ = (MinZ <= MaxZ) ? (Local.Z >= MinZ && Local.Z <= MaxZ) : false;

	return Dist2D <= AllowedRadius && bInsideZ;
}

void ACombatZoneActor::SetZoneRadius(float InRadius)
{
	if (!ZoneBounds) return;
	ZoneBounds->SetSphereRadius(FMath::Max(0.0f, InRadius));
}

void ACombatZoneActor::SetZoneHalfHeight(float InHalfHeight)
{
	ZoneHalfHeight = FMath::Max(0.0f, InHalfHeight);
}

void ACombatZoneActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDrawDebug || !ZoneBounds) return;

	const FTransform T = ZoneBounds->GetComponentTransform();
	const FVector Scale3D = T.GetScale3D().GetAbs();
	const float Radius = ZoneBounds->GetUnscaledSphereRadius() * FMath::Max(Scale3D.X, Scale3D.Y);

	DrawDebugSphere(
		GetWorld(),
		T.GetLocation(),
		Radius,
		48,
		FColor::Cyan,
		false,
		0.0f,
		0,
		DebugDrawThickness
	);
}
