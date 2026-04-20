#include "Graphics/FakeGIActor.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFakeGIActor::AFakeGIActor()
{
    PrimaryActorTick.bCanEverTick = false;

    GIMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GIMesh"));
    SetRootComponent(GIMesh);

    GIMesh->SetCastShadow(false);
    GIMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GIMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    GIMesh->SetHiddenInGame(true);
    GIMesh->SetAffectIndirectLightingWhileHidden(true);


    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        GIMesh->SetStaticMesh(SphereMesh.Object);
    }
}

void AFakeGIActor::BeginPlay()
{
    Super::BeginPlay();

    UpdateFakeGI();
}

void AFakeGIActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    UpdateFakeGI();
}

#if WITH_EDITOR
void AFakeGIActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    UpdateFakeGI();
}
#endif

void AFakeGIActor::UpdateFakeGI()
{
    ApplyMeshType();
    EnsureDynamicMaterial();
    ApplyAngle();
    ApplyRange();
    ApplyIntensity();
    ApplyEnabled();
}

void AFakeGIActor::EnsureDynamicMaterial()
{
    if (!GIMesh)
    {
        return;
    }

    if (DynamicMaterial && GIMesh->GetMaterial(0) == DynamicMaterial)
    {
        return;
    }

    if (DynamicMaterial)
    {
        GIMesh->SetMaterial(0, DynamicMaterial);
        return;
    }

    if (UMaterialInterface* Material = GIMesh->GetMaterial(0))
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
        GIMesh->SetMaterial(0, DynamicMaterial);
    }
}

void AFakeGIActor::ApplyMeshType()
{
    UStaticMesh* TargetMesh = nullptr;

    auto LoadMesh = [](const TCHAR* Path) -> UStaticMesh*
        {
            return Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, Path));
        };

    switch (MeshType)
    {
    case EFakeGIMeshType::Sphere:
        TargetMesh = LoadMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        break;
    case EFakeGIMeshType::Plane:
        TargetMesh = LoadMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
        break;
    case EFakeGIMeshType::Cube:
        TargetMesh = LoadMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
        break;
    case EFakeGIMeshType::Cylinder:
        TargetMesh = LoadMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        break;
    case EFakeGIMeshType::Custom:
        TargetMesh = CustomMesh;
        break;
    }

    if (TargetMesh)
    {
        GIMesh->SetStaticMesh(TargetMesh);
    }
}

void AFakeGIActor::ApplyAngle()
{
    GIMesh->SetWorldRotation(FRotator(LightPitch, LightYaw, 0.f));
}

void AFakeGIActor::ApplyRange()
{
   
    const float Scale = FMath::Max(GIRange / 1000.f, 0.1f);
    GIMesh->SetWorldScale3D(FVector(Scale));
}

void AFakeGIActor::ApplyIntensity()
{
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(FName("GI_Intensity"), GIIntensity);
        DynamicMaterial->SetVectorParameterValue(FName("GI_Color"), GIColor);
    }
}

// 적용 여부 
void AFakeGIActor::ApplyEnabled()
{
    if (DynamicMaterial)
    {
        const float FinalIntensity = bFakeGIEnabled ? GIIntensity : 0.f;
        DynamicMaterial->SetScalarParameterValue(FName("GI_Intensity"), FinalIntensity);
    }
}

void AFakeGIActor::SetFakeGIEnabled(bool bEnabled)
{
    bFakeGIEnabled = bEnabled;
    ApplyEnabled();
}

