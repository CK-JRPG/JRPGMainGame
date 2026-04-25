#include "Combat/Session/CombatZoneActor.h"

#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

ACombatZoneActor::ACombatZoneActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	SetRootComponent(ZoneBounds);

	// 존 추적은 파티/적 커스텀 채널 모두를 받아야 한다.
	ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneBounds->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	ZoneBounds->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
	ZoneBounds->SetGenerateOverlapEvents(true);

	ZoneBounds->SetBoxExtent(FVector(1200.f, 1200.f, 300.f));
	
	bDrawDebug = true;
}

FVector ACombatZoneActor::ClampCharacterLocation(const FVector& WorldLocation, float CapsuleRadius, float CapsuleHalfHeight) const
{
	if (!ZoneBounds) return WorldLocation;

	const FTransform T = ZoneBounds->GetComponentTransform();
	const FVector Ext = ZoneBounds->GetScaledBoxExtent();

	// 월드 -> 로컬
	FVector Local = T.InverseTransformPosition(WorldLocation);

	const float MinX = -Ext.X + CapsuleRadius;
	const float MaxX =  Ext.X - CapsuleRadius;
	const float MinY = -Ext.Y + CapsuleRadius;
	const float MaxY =  Ext.Y - CapsuleRadius;
	const float MinZ = -Ext.Z + CapsuleHalfHeight;
	const float MaxZ =  Ext.Z - CapsuleHalfHeight;

	Local.X = FMath::Clamp(Local.X, MinX, MaxX);
	Local.Y = FMath::Clamp(Local.Y, MinY, MaxY);
	Local.Z = FMath::Clamp(Local.Z, MinZ, MaxZ);

	// 로컬 -> 월드
	return T.TransformPosition(Local);
}

bool ACombatZoneActor::IsCharacterInside(const FVector& WorldLocation, float CapsuleRadius, float CapsuleHalfHeight) const
{
	if (!ZoneBounds) return false;

	const FTransform T = ZoneBounds->GetComponentTransform();
	const FVector Ext = ZoneBounds->GetScaledBoxExtent();

	const FVector Local = T.InverseTransformPosition(WorldLocation);

	const float MinX = -Ext.X + CapsuleRadius;
	const float MaxX =  Ext.X - CapsuleRadius;
	const float MinY = -Ext.Y + CapsuleRadius;
	const float MaxY =  Ext.Y - CapsuleRadius;
	const float MinZ = -Ext.Z + CapsuleHalfHeight;
	const float MaxZ =  Ext.Z - CapsuleHalfHeight;

	return (Local.X >= MinX && Local.X <= MaxX
		&&  Local.Y >= MinY && Local.Y <= MaxY
		&&  Local.Z >= MinZ && Local.Z <= MaxZ);
}

void ACombatZoneActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDrawDebug || !ZoneBounds) return;

	const FTransform T = ZoneBounds->GetComponentTransform();
	const FVector Ext = ZoneBounds->GetScaledBoxExtent();

	DrawDebugBox(
		GetWorld(),
		T.GetLocation(),
		Ext,
		T.GetRotation(),
		FColor::Cyan,
		false,
		0.0f,
		0,
		DebugDrawThickness
	);
}
