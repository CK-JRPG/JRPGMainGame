// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "FakeGIActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EFakeGIMeshType : uint8
{
    Sphere      UMETA(DisplayName = "Sphere"),
    Plane       UMETA(DisplayName = "Plane"),
    Cube        UMETA(DisplayName = "Cube"),
    Cylinder    UMETA(DisplayName = "Cylinder"),
    Custom      UMETA(DisplayName = "Custom"),
};

UCLASS()
class JRPG_API AFakeGIActor : public AActor
{
    GENERATED_BODY()

public:
    AFakeGIActor();

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fake GI")
    TObjectPtr<UStaticMeshComponent> GIMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Mesh")
    EFakeGIMeshType MeshType = EFakeGIMeshType::Sphere;

    // 메시 다른거 쓸거면 이거 사용
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Mesh",
        meta = (EditCondition = "MeshType == EFakeGIMeshType::Custom"))
    TObjectPtr<UStaticMesh> CustomMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI")
    bool bFakeGIEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Material")
    TObjectPtr<UMaterialInterface> FakeGIMaterial;


    //각도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Angle",
        meta = (ClampMin = "-180.0", ClampMax = "180.0"))
    float LightYaw = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Angle",
        meta = (ClampMin = "-90.0", ClampMax = "90.0"))
    float LightPitch = -45.f;

    // 메시 스케일로 이미시브 라이트 범위 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Range",
        meta = (ClampMin = "1.0", ClampMax = "50000.0"))
    float GIRange = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Culling")
    bool bNeverCullGIProxy = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Culling",
        meta = (ClampMin = "1.0", ClampMax = "10000.0"))
    float GIBoundsScale = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake GI|Intensity",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float GIIntensity = 1.f;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Fake GI|Intensity")
    FLinearColor GIColor = FLinearColor(1.f, 0.9f, 0.7f, 1.f);

    UFUNCTION(BlueprintCallable, Category = "Fake GI")
    void UpdateFakeGI();

    UFUNCTION(BlueprintCallable, Category = "Fake GI")
    void SetFakeGIEnabled(bool bEnabled);

private:
    void EnsureDynamicMaterial();
    UMaterialInterface* ResolveSourceMaterial() const;
    void ApplyMeshType();
    void ApplyAngle();
    void ApplyRange();
    void ApplyCulling();
    void ApplyIntensity();
    void ApplyEnabled();

    UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial = nullptr;
};
