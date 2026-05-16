#include "Combat/Battle/DirectionalDamageComponent.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Skills/SkillDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogJRPGDirectionalDamage, Log, All);

UDirectionalDamageComponent::UDirectionalDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FDirectionalDamageResult UDirectionalDamageComponent::EvaluateDirectionalDamage(AActor* SourceActor) const
{
	return EvaluateDirectionalDamageWithSettings(
		SourceActor,
		BackDotThreshold,
		SideDotThreshold,
		BackDamageMultiplier,
		SideDamageMultiplier);
}

FDirectionalDamageResult UDirectionalDamageComponent::EvaluateDirectionalDamageWithSettings(
	AActor* SourceActor,
	float InBackDotThreshold,
	float InSideDotThreshold,
	float InBackDamageMultiplier,
	float InSideDamageMultiplier) const
{
	FDirectionalDamageResult Result;

	const AActor* OwnerActor = GetOwner();
	if (!bEnableDirectionalDamage || !OwnerActor || !SourceActor)
	{
		return Result;
	}

	FVector ToSource = SourceActor->GetActorLocation() - OwnerActor->GetActorLocation();
	ToSource.Z = 0.f;
	if (!ToSource.Normalize())
	{
		return Result;
	}

	FVector Forward = OwnerActor->GetActorForwardVector();
	Forward.Z = 0.f;
	if (!Forward.Normalize())
	{
		return Result;
	}

	FVector Right = OwnerActor->GetActorRightVector();
	Right.Z = 0.f;
	if (!Right.Normalize())
	{
		return Result;
	}

	Result.ForwardDot = FVector::DotProduct(Forward, ToSource);
	Result.RightDot = FVector::DotProduct(Right, ToSource);

	const float ClampedBackThreshold = FMath::Clamp(InBackDotThreshold, -1.f, 1.f);
	const float ClampedSideThreshold = FMath::Clamp(InSideDotThreshold, 0.f, 1.f);

	if (Result.ForwardDot <= ClampedBackThreshold)
	{
		Result.Side = EDirectionalDamageSide::Back;
		Result.DamageMultiplier = FMath::Max(1.f, InBackDamageMultiplier);
	}
	else if (FMath::Abs(Result.RightDot) >= ClampedSideThreshold)
	{
		Result.Side = Result.RightDot >= 0.f ? EDirectionalDamageSide::Right : EDirectionalDamageSide::Left;
		Result.DamageMultiplier = FMath::Max(1.f, InSideDamageMultiplier);
	}
	else
	{
		Result.Side = EDirectionalDamageSide::Front;
		Result.DamageMultiplier = 1.f;
	}

	Result.bAppliesDamageBonus = Result.DamageMultiplier > 1.f;
	return Result;
}

float UDirectionalDamageComponent::EvaluateSkillDamageMultiplier(const USkillDataAsset* Skill, AActor* SourceActor) const
{
	if (!Skill)
	{
		return 1.f;
	}

	if (!Skill->bEnableDirectionalDamageBonus)
	{
		return 1.f;
	}

	const FString SkillId = Skill->SkillId.ToString();
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !SourceActor)
	{
		UE_LOG(LogJRPGDirectionalDamage, Log,
			TEXT("[DirectionalDamage] NotApplied | Reason=InvalidActor Skill=%s Source=%s Target=%s"),
			*SkillId,
			*GetNameSafe(SourceActor),
			*GetNameSafe(OwnerActor));
		return 1.f;
	}

	const ICombatParticipantInterface* SourceParticipant = Cast<ICombatParticipantInterface>(SourceActor);
	const ICombatParticipantInterface* TargetParticipant = Cast<ICombatParticipantInterface>(OwnerActor);
	if (!SourceParticipant
		|| !TargetParticipant
		|| SourceParticipant->GetCombatTeam() != ECombatTeam::Player
		|| TargetParticipant->GetCombatTeam() != ECombatTeam::Enemy
		|| !SourceParticipant->IsPlayerControlledCombatant())
	{
		const int32 SourceTeam = SourceParticipant ? static_cast<int32>(SourceParticipant->GetCombatTeam()) : -1;
		const int32 TargetTeam = TargetParticipant ? static_cast<int32>(TargetParticipant->GetCombatTeam()) : -1;
		const bool bSourcePlayerControlled = SourceParticipant && SourceParticipant->IsPlayerControlledCombatant();

		UE_LOG(LogJRPGDirectionalDamage, Log,
			TEXT("[DirectionalDamage] NotApplied | Reason=Participant Skill=%s Source=%s Target=%s SourceTeam=%d TargetTeam=%d PlayerControlled=%s"),
			*SkillId,
			*GetNameSafe(SourceActor),
			*GetNameSafe(OwnerActor),
			SourceTeam,
			TargetTeam,
			bSourcePlayerControlled ? TEXT("true") : TEXT("false"));
		return 1.f;
	}

	const UCombatCharacterComponent* SourceCharacter = SourceActor->FindComponentByClass<UCombatCharacterComponent>();
	if (!SourceCharacter || SourceCharacter->GetRole() != EJRPGPartyRole::Attacker)
	{
		const int32 SourceRole = SourceCharacter ? static_cast<int32>(SourceCharacter->GetRole()) : -1;

		UE_LOG(LogJRPGDirectionalDamage, Log,
			TEXT("[DirectionalDamage] NotApplied | Reason=Role Skill=%s Source=%s Target=%s SourceRole=%d"),
			*SkillId,
			*GetNameSafe(SourceActor),
			*GetNameSafe(OwnerActor),
			SourceRole);
		return 1.f;
	}

	const FDirectionalDamageResult DirectionalResult = EvaluateDirectionalDamageWithSettings(
		SourceActor,
		Skill->DirectionalBackDotThreshold,
		Skill->DirectionalSideDotThreshold,
		Skill->DirectionalBackDamageMultiplier,
		Skill->DirectionalSideDamageMultiplier);
	const float Multiplier = DirectionalResult.bAppliesDamageBonus ? DirectionalResult.DamageMultiplier : 1.f;
	const TCHAR* SideName = TEXT("None");
	switch (DirectionalResult.Side)
	{
	case EDirectionalDamageSide::Front:
		SideName = TEXT("Front");
		break;
	case EDirectionalDamageSide::Back:
		SideName = TEXT("Back");
		break;
	case EDirectionalDamageSide::Left:
		SideName = TEXT("Left");
		break;
	case EDirectionalDamageSide::Right:
		SideName = TEXT("Right");
		break;
	default:
		break;
	}

	UE_LOG(LogJRPGDirectionalDamage, Log,
		TEXT("[DirectionalDamage] Evaluated | Skill=%s Source=%s Target=%s Side=%s Multiplier=%.2f Applies=%s ForwardDot=%.3f RightDot=%.3f"),
		*SkillId,
		*GetNameSafe(SourceActor),
		*GetNameSafe(OwnerActor),
		SideName,
		Multiplier,
		DirectionalResult.bAppliesDamageBonus ? TEXT("true") : TEXT("false"),
		DirectionalResult.ForwardDot,
		DirectionalResult.RightDot);

	return Multiplier;
}
