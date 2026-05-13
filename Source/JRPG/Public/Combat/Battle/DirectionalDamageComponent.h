#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DirectionalDamageComponent.generated.h"

class USkillDataAsset;

UENUM(BlueprintType)
enum class EDirectionalDamageSide : uint8
{
	None,
	Front,
	Back,
	Left,
	Right
};

USTRUCT(BlueprintType)
struct FDirectionalDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bAppliesDamageBonus = false;
	UPROPERTY(BlueprintReadOnly) EDirectionalDamageSide Side = EDirectionalDamageSide::None;
	UPROPERTY(BlueprintReadOnly) float DamageMultiplier = 1.f;
	UPROPERTY(BlueprintReadOnly) float ForwardDot = 0.f;
	UPROPERTY(BlueprintReadOnly) float RightDot = 0.f;
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UDirectionalDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDirectionalDamageComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Damage")
	bool bEnableDirectionalDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Damage", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float BackDotThreshold = -0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Damage", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SideDotThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Damage", meta=(ClampMin="1.0"))
	float BackDamageMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Damage", meta=(ClampMin="1.0"))
	float SideDamageMultiplier = 1.5f;

	UFUNCTION(BlueprintPure, Category="Directional Damage")
	FDirectionalDamageResult EvaluateDirectionalDamage(AActor* SourceActor) const;

	float EvaluateSkillDamageMultiplier(const USkillDataAsset* Skill, AActor* SourceActor) const;
};
