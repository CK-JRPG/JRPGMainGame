#include "Combat/Presentation/CombatVFXComponent.h"

#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Skills/SkillComponent.h"
#include "Combat/Skills/SkillDataAsset.h"

#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

UCombatVFXComponent::UCombatVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatVFXComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	CharacterComp = Owner ? Owner->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
	PresentationComp = Owner ? Owner->FindComponentByClass<UCombatPresentationComponent>() : nullptr;
	SkillComp = Owner ? Owner->FindComponentByClass<USkillComponent>() : nullptr;

	if (SkillComp.IsValid())
	{
		SkillComp->OnSkillTargetResolved.AddUObject(this, &UCombatVFXComponent::HandleSkillTargetResolved);
	}

	if (UBasicCombatSubsystem* Basic = GetWorld() ? GetWorld()->GetSubsystem<UBasicCombatSubsystem>() : nullptr)
	{
		Basic->OnBasicAttackResolved.AddUObject(this, &UCombatVFXComponent::HandleBasicAttackResolved);
	}
}

void UCombatVFXComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SkillComp.IsValid())
	{
		SkillComp->OnSkillTargetResolved.RemoveAll(this);
	}

	if (UBasicCombatSubsystem* Basic = GetWorld() ? GetWorld()->GetSubsystem<UBasicCombatSubsystem>() : nullptr)
	{
		Basic->OnBasicAttackResolved.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UCombatVFXComponent::HandleBasicAttackResolved(const FCombatActionResult& Result)
{
	AActor* Owner = GetOwner();
	if (!Result.bOk || !IsValid(Owner) || Result.Attacker.Get() != Owner || !IsValid(Result.Target.Get()))
	{
		return;
	}

	const UCombatCharacterDataAsset* CharacterDef = CharacterComp.IsValid() ? CharacterComp->GetCharacterData() : nullptr;
	if (!CharacterDef || !CharacterDef->BasicAttackHitNiagaraEffect)
	{
		return;
	}

	const float Scale = EvaluateEffectScale(
		Result.Breakdown.FinalDamage,
		CharacterDef->BasicAttackDamageToEffectScaleCurve,
		CharacterDef->BasicAttackEffectScaleReferenceDamage,
		CharacterDef->BasicAttackEffectScaleMultiplier,
		CharacterDef->BasicAttackMinEffectScale,
		CharacterDef->BasicAttackMaxEffectScale);

	SpawnTargetEffect(CharacterDef->BasicAttackHitNiagaraEffect, Result.Target.Get(), Scale, Result.Breakdown.bCritical);
}

void UCombatVFXComponent::HandleSkillTargetResolved(const FSkillTargetResolvedEvent& Event)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || Event.Caster.Get() != Owner || !IsValid(Event.Target.Get()) || !SkillComp.IsValid())
	{
		return;
	}

	USkillDataAsset* Skill = SkillComp->GetSkillDef(Event.SkillId);
	if (!Skill)
	{
		return;
	}

	UNiagaraSystem* TargetEffect = nullptr;
	if (Event.FinalDamage > 0.f)
	{
		TargetEffect = Skill->HitNiagaraEffect ? Skill->HitNiagaraEffect.Get() : Skill->OnResolveTargetEffect.Get();
	}
	else if (Event.HealAmount > 0.f || Event.bStatusApplied || Event.StatusRemovedCount > 0)
	{
		TargetEffect = Skill->OnResolveTargetEffect.Get();
	}
	else
	{
		TargetEffect = Skill->OnResolveTargetEffect.Get();
	}

	const float Magnitude = FMath::Max(Event.FinalDamage, Event.HealAmount);
	const float Scale = EvaluateEffectScale(
		Magnitude,
		Skill->DamageToEffectScaleCurve,
		Skill->EffectScaleReferenceDamage,
		Skill->EffectScaleMultiplier,
		Skill->MinEffectScale,
		Skill->MaxEffectScale);

	if (TargetEffect)
	{
		SpawnTargetEffect(TargetEffect, Event.Target.Get(), Scale, Event.bCritical);
	}

	if (Skill->OnResolveGroundEffect)
	{
		UNiagaraComponent* GroundComp = SpawnTargetEffect(Skill->OnResolveGroundEffect, Event.Target.Get(), 1.f, Event.bCritical);
		if (GroundComp && Skill->GroundEffectDuration > 0.f && GetWorld())
		{
			TWeakObjectPtr<UNiagaraComponent> WeakGroundComp = GroundComp;
			FTimerHandle GroundEffectTimer;
			GetWorld()->GetTimerManager().SetTimer(
				GroundEffectTimer,
				[WeakGroundComp]()
				{
					if (WeakGroundComp.IsValid())
					{
						WeakGroundComp->Deactivate();
					}
				},
				Skill->GroundEffectDuration,
				false);
		}
	}
}

UNiagaraComponent* UCombatVFXComponent::SpawnTargetEffect(UNiagaraSystem* System, AActor* Target, float UniformScale, bool bCritical) const
{
	if (!System || !IsValid(Target) || !GetWorld())
	{
		return nullptr;
	}

	FVector SurfaceNormal = FVector::UpVector;
	const FVector Location = ResolveTargetEffectLocation(Target);
	const FRotator Rotation = ResolveTargetEffectRotation(Target, SurfaceNormal);
	const FVector SpawnLocation = Location + SurfaceNormal * 12.f;
	const FVector Scale(UniformScale);

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		System,
		SpawnLocation,
		Rotation,
		Scale,
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true);

	if (NiagaraComp)
	{
		NiagaraComp->SetVariableFloat(TEXT("DamageScale"), UniformScale);
		NiagaraComp->SetVariableBool(TEXT("bIsCrit"), bCritical);
	}

	return NiagaraComp;
}

float UCombatVFXComponent::EvaluateEffectScale(float Magnitude, const UCurveFloat* Curve, float ReferenceDamage, float Multiplier, float MinScale, float MaxScale) const
{
	const float SafeMin = FMath::Max(1.f, MinScale);
	const float SafeMax = FMath::Max(SafeMin, MaxScale);

	float Scale = SafeMin;
	if (Curve)
	{
		Scale = Curve->GetFloatValue(FMath::Max(0.f, Magnitude));
	}
	else
	{
		const float SafeReference = FMath::Max(1.f, ReferenceDamage);
		Scale = 1.f + (FMath::Max(0.f, Magnitude) / SafeReference) * FMath::Max(0.f, Multiplier);
	}

	return FMath::Clamp(Scale, SafeMin, SafeMax);
}

FVector UCombatVFXComponent::ResolveTargetEffectLocation(AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (Mesh->DoesSocketExist(TEXT("root")))
			{
				return Mesh->GetSocketLocation(TEXT("root"));
			}

			const FBoxSphereBounds Bounds = Mesh->Bounds;
			return Bounds.Origin;
		}
	}

	return Target->GetActorLocation();
}

FRotator UCombatVFXComponent::ResolveTargetEffectRotation(AActor* Target, FVector& OutSurfaceNormal) const
{
	AActor* Owner = GetOwner();
	if (Owner && Target)
	{
		const FVector Direction = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
		if (!Direction.IsNearlyZero())
		{
			OutSurfaceNormal = Direction;
			return Direction.Rotation();
		}
	}

	OutSurfaceNormal = FVector::UpVector;
	return FRotator::ZeroRotator;
}
