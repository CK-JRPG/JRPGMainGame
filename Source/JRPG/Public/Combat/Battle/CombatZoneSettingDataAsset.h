// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatZoneSettingDataAsset.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ECombatZoneShape : uint8
{
    Sphere  UMETA(DisplayName = "Sphere"),
    Box     UMETA(DisplayName = "Box"),
};

class AActor;
class ACombatZoneActor;

UCLASS()
class JRPG_API UCombatZoneSettingDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "CombatZone|Shape")
    ECombatZoneShape ZoneShape = ECombatZoneShape::Sphere;

    // Sphere일 때 사용
    UPROPERTY(EditAnywhere, Category = "CombatZone|Size")
    float ZoneSphereRadius = 1000.f;

    // Box일 때 사용
    UPROPERTY(EditAnywhere, Category = "CombatZone|Size")
    FVector ZoneBoxExtent = FVector(1200.f, 1200.f, 300.f);

    UPROPERTY(EditAnywhere, Category = "CombatZone|PawnClass")
    TSubclassOf<AActor> FieldPlayerPawnClass;

    UPROPERTY(EditAnywhere, Category = "CombatZone|Class")
    TSubclassOf<ACombatZoneActor> CombatZoneClass;
};