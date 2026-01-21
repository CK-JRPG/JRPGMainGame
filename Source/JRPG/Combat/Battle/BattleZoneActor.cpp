#include "BattleZoneActor.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"

ABattleZoneActor::ABattleZoneActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>("Root");
    RootComponent = Root;

    ZoneDecal = CreateDefaultSubobject<UDecalComponent>("ZoneDecal");
    ZoneDecal->SetupAttachment(RootComponent);
    ZoneDecal->SetHiddenInGame(true);
    ZoneDecal->DecalSize = FVector(200.f, 10000.f, 10000.f);
}

void ABattleZoneActor::BeginPlay()
{
    Super::BeginPlay();
    if (Center.IsNearlyZero()) Center = GetActorLocation();
    UpdateVisual();
}

void ABattleZoneActor::RegisterParticipant(AActor* Actor)
{
    if (!Actor) return;
    Participants.AddUnique(Actor);
}

void ABattleZoneActor::UpdateVisual()
{
    if (!ZoneDecal) return;

    if (ZoneDecalMaterial)
    {
        ZoneDecal->SetDecalMaterial(ZoneDecalMaterial);
        ZoneDecal->SetHiddenInGame(false);

        const float R = Radius;
        ZoneDecal->DecalSize = FVector(200.f, R, R);
        ZoneDecal->SetWorldLocation(Center + FVector(0,0,5));
        ZoneDecal->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));
    }
    else
    {
        ZoneDecal->SetHiddenInGame(true);
    }
}

void ABattleZoneActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDrawDebugCircle)
    {
        DrawDebugCircle(
            GetWorld(),
            Center,
            Radius,
            96,
            FColor::Green,
            false,
            -1.f,
            0,
            DebugCircleThickness,
            FVector(1,0,0),
            FVector(0,1,0),
            false
        );
    }

    for (int32 i = Participants.Num() - 1; i >= 0; --i)
    {
        AActor* A = Participants[i].Get();
        if (!A) { Participants.RemoveAt(i); continue; }
        ClampActor(A);
    }
}

void ABattleZoneActor::ClampActor(AActor* Actor)
{
    const FVector Pos = Actor->GetActorLocation();
    FVector Offset = Pos - Center;
    Offset.Z = 0.f;

    const float Dist = Offset.Length();
    const float MaxDist = FMath::Max(0.f, Radius - SafetyMargin);

    if (Dist > MaxDist && Dist > KINDA_SMALL_NUMBER)
    {
        //log
        if (GEngine) GEngine->AddOnScreenDebugMessage(123, 0.1f, FColor::Magenta, FString::Printf(TEXT("!!! WALL BLOCKING: %s !!!"), *Actor->GetName()));

        const FVector Dir = Offset / Dist;
        FVector NewPos = Center + Dir * MaxDist;
        NewPos.Z = Pos.Z;
        Actor->SetActorLocation(NewPos, false, nullptr, ETeleportType::TeleportPhysics);
    }
}
