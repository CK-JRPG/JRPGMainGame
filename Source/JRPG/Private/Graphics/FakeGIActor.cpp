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
    GIMesh->SetCullDistance(0.f);
    GIMesh->bAllowCullDistanceVolume = false;
    GIMesh->SetBoundsScale(GIBoundsScale);
    GIMesh->SetVisibleInRayTracing(true);
    GIMesh->RayTracingGroupCullingPriority = ERayTracingGroupCullingPriority::CP_0_NEVER_CULL;


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

    RefreshFakeGI(false);
}

void AFakeGIActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITOR
    if (!bIsApplyingEditorPropertyChange)
    {
        SyncTransformPropertiesFromActor();
    }
#endif

    RefreshFakeGI(false);
}

#if WITH_EDITOR
void AFakeGIActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const bool bTransformPropertyChanged = IsTransformPropertyChanged(PropertyChangedEvent);

    bIsApplyingEditorPropertyChange = true;
    Super::PostEditChangeProperty(PropertyChangedEvent);
    bIsApplyingEditorPropertyChange = false;

    if (bTransformPropertyChanged)
    {
        UpdateFakeGI();
    }
    else
    {
        RefreshFakeGI(false);
    }
}

void AFakeGIActor::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    SyncTransformPropertiesFromActor();
    RefreshFakeGI(false);
}
#endif

void AFakeGIActor::UpdateFakeGI()
{
    RefreshFakeGI(true);
}

void AFakeGIActor::RefreshFakeGI(bool bApplyTransform)
{
    ApplyMeshType();
    EnsureDynamicMaterial();
    if (bApplyTransform)
    {
        ApplyAngle();
        ApplyRange();
    }
    ApplyCulling();
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
    SetActorRotation(FRotator(LightPitch, LightYaw, LightRoll));
}

void AFakeGIActor::ApplyRange()
{
    if (!GIMesh)
    {
        return;
    }

    FVector TargetSize(GIRange);
    if (bUseCustomGISize)
    {
        TargetSize = GISize;
    }

    TargetSize.X = FMath::Clamp(TargetSize.X, 1.f, 50000.f);
    TargetSize.Y = FMath::Clamp(TargetSize.Y, 1.f, 50000.f);
    TargetSize.Z = FMath::Clamp(TargetSize.Z, 1.f, 50000.f);

    SetActorScale3D(TargetSize / 1000.f);
}

void AFakeGIActor::ApplyCulling()
{
    if (!GIMesh)
    {
        return;
    }

    if (bNeverCullGIProxy)
    {
        GIMesh->SetCullDistance(0.f);
        GIMesh->bAllowCullDistanceVolume = false;
        GIMesh->SetVisibleInRayTracing(true);
        GIMesh->RayTracingGroupCullingPriority = ERayTracingGroupCullingPriority::CP_0_NEVER_CULL;
    }

    GIMesh->SetBoundsScale(FMath::Max(GIBoundsScale, 1.f));
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

void AFakeGIActor::SetGISize(const FVector& NewSize)
{
    bUseCustomGISize = true;
    GISize.X = FMath::Clamp(NewSize.X, 1.f, 50000.f);
    GISize.Y = FMath::Clamp(NewSize.Y, 1.f, 50000.f);
    GISize.Z = FMath::Clamp(NewSize.Z, 1.f, 50000.f);
    ApplyRange();
    ApplyCulling();
}

#if WITH_EDITOR
void AFakeGIActor::SyncTransformPropertiesFromActor()
{
    const FRotator ActorRotation = GetActorRotation();
    LightPitch = ActorRotation.Pitch;
    LightYaw = ActorRotation.Yaw;
    LightRoll = ActorRotation.Roll;

    const FVector ActorScale = GetActorScale3D();
    FVector ActorSize(
        FMath::Clamp(ActorScale.X * 1000.f, 1.f, 50000.f),
        FMath::Clamp(ActorScale.Y * 1000.f, 1.f, 50000.f),
        FMath::Clamp(ActorScale.Z * 1000.f, 1.f, 50000.f));

    const bool bUniformScale =
        FMath::IsNearlyEqual(ActorSize.X, ActorSize.Y, 0.1f) &&
        FMath::IsNearlyEqual(ActorSize.Y, ActorSize.Z, 0.1f);

    GIRange = FMath::Clamp(ActorSize.GetMax(), 1.f, 50000.f);
    GISize = ActorSize;

    if (!bUniformScale)
    {
        bUseCustomGISize = true;
    }
}

bool AFakeGIActor::IsTransformPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent) const
{
    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;
    const FName MemberPropertyName = PropertyChangedEvent.MemberProperty
        ? PropertyChangedEvent.MemberProperty->GetFName()
        : NAME_None;

    auto Matches = [PropertyName, MemberPropertyName](const FName Name)
        {
            return PropertyName == Name || MemberPropertyName == Name;
        };

    return Matches(GET_MEMBER_NAME_CHECKED(AFakeGIActor, LightYaw)) ||
        Matches(GET_MEMBER_NAME_CHECKED(AFakeGIActor, LightPitch)) ||
        Matches(GET_MEMBER_NAME_CHECKED(AFakeGIActor, LightRoll)) ||
        Matches(GET_MEMBER_NAME_CHECKED(AFakeGIActor, GIRange)) ||
        Matches(GET_MEMBER_NAME_CHECKED(AFakeGIActor, bUseCustomGISize)) ||
        Matches(GET_MEMBER_NAME_CHECKED(AFakeGIActor, GISize));
}
#endif
