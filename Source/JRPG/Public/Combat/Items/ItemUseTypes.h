// Source/JRPGCombat/Public/Combat/Items/ItemUseTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ItemUseTypes.generated.h"

UENUM()
enum class EItemUseTargeting : uint8
{
	Self,
	AllySingle,
	EnemySingle,
	PartyAll
};

UENUM()
enum class EConsumableEffectType : uint8
{
	HealHPFlat,
	HealHPPctMax,
	RestoreAPFlat,
	CleanseStatusId,
	CleanseByTag,
	ApplyStatusId,
	GrantSPFlat
};

USTRUCT()
struct FConsumableEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EConsumableEffectType Type = EConsumableEffectType::HealHPFlat;

	UPROPERTY(EditAnywhere) float Value = 0.f;

	UPROPERTY(EditAnywhere) FName StatusId = NAME_None;
	UPROPERTY(EditAnywhere) FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere) float DurationSec = 0.f;
	UPROPERTY(EditAnywhere) float Magnitude = 1.f;
	UPROPERTY(EditAnywhere) int32 Stacks = 1;

	UPROPERTY(EditAnywhere) FName SourceTag = NAME_None;
};