#include "Combat/Presentation/TargetGuideLineComponent.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "Combat/Core/RoleTypes.h"
#include "Combat/Stats/HPComponent.h"

#include "Components/LineBatchComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "SceneTypes.h"

UTargetGuideLineComponent::UTargetGuideLineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTargetGuideLineComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	OwnerCharacterComp = Owner ? Owner->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
	OwnerHPComp = Owner ? Owner->FindComponentByClass<UHPComponent>() : nullptr;
	OwnerMeshComp = nullptr;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
	{
		OwnerMeshComp = OwnerCharacter->GetMesh();
	}

	if (!OwnerCharacterComp.IsValid() || OwnerCharacterComp->GetTeam() != ECombatTeam::Enemy)
	{
		SetComponentTickEnabled(false);
		return;
	}
}

void UTargetGuideLineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AggroTarget = nullptr;
	TargetCharacterComp = nullptr;
	TargetHPComp = nullptr;
	TargetMeshComp = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UTargetGuideLineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateGuideLine();
}

void UTargetGuideLineComponent::SetAggroTarget(AActor* NewTarget)
{
	if (AggroTarget.Get() == NewTarget)
	{
		UpdateGuideLine();
		return;
	}

	AggroTarget = NewTarget;
	CacheTargetData(NewTarget);
	UpdateGuideLine();
}

void UTargetGuideLineComponent::ClearAggroTarget()
{
	AggroTarget = nullptr;
	TargetCharacterComp = nullptr;
	TargetHPComp = nullptr;
	TargetMeshComp = nullptr;
	SetComponentTickEnabled(false);
}

void UTargetGuideLineComponent::CacheTargetData(AActor* Target)
{
	TargetCharacterComp = Target ? Target->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
	TargetHPComp = Target ? Target->FindComponentByClass<UHPComponent>() : nullptr;
	TargetMeshComp = nullptr;
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		TargetMeshComp = TargetCharacter->GetMesh();
	}
}

void UTargetGuideLineComponent::UpdateGuideLine()
{
	AActor* Target = AggroTarget.Get();
	if (!bUseLineBatchGuide || !CanShowGuideTo(Target))
	{
		SetComponentTickEnabled(false);
		return;
	}

	const FVector Start = ResolveOwnerGuideLocation();
	const FVector End = ResolveTargetGuideLocation(Target);
	const float Distance = FVector::Distance(Start, End);
	FVector ControlPoint = (Start + End) * 0.5f;
	ControlPoint.Z += FMath::Clamp(Distance * ArcHeightByDistance, MinArcHeight, MaxArcHeight);

	const FLinearColor GuideColor = ResolveGuideColor(Target);

	DrawLineBatchGuide(Start, ControlPoint, End, GuideColor);

	SetComponentTickEnabled(true);
}

void UTargetGuideLineComponent::DrawLineBatchGuide(const FVector& Start, const FVector& ControlPoint, const FVector& End, const FLinearColor& GuideColor) const
{
	const UWorld* World = GetWorld();
	ULineBatchComponent* LineBatcher = World ? World->GetLineBatcher(UWorld::ELineBatcherType::Foreground) : nullptr;
	if (!LineBatcher)
	{
		return;
	}

	const int32 SegmentCount = FMath::Clamp(LineSegmentCount, 2, 64);
	const float LifeTime = FMath::Max(LineBatchLifetime, 0.01f);
	const float Thickness = FMath::Max(LineWidth, 1.0f);
	FLinearColor Color = GuideColor;
	Color.A *= Alpha;

	auto EvalQuadraticBezier = [&Start, &ControlPoint, &End](float T)
		{
			const float InvT = 1.0f - T;
			return InvT * InvT * Start + 2.0f * InvT * T * ControlPoint + T * T * End;
		};

	FVector PreviousPoint = Start;
	for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float T = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const FVector CurrentPoint = EvalQuadraticBezier(T);
		LineBatcher->DrawLine(PreviousPoint, CurrentPoint, Color, SDPG_Foreground, Thickness, LifeTime);
		PreviousPoint = CurrentPoint;
	}
}

bool UTargetGuideLineComponent::CanShowGuideTo(AActor* Target) const
{
	const AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !IsValid(Target) || Owner == Target)
	{
		return false;
	}

	if (IsActorDead(Owner))
	{
		return false;
	}

	if (IsActorDead(Target))
	{
		return false;
	}

	const UCombatCharacterComponent* OwnerComp = OwnerCharacterComp.Get();
	if (!OwnerComp || OwnerComp->GetTeam() != ECombatTeam::Enemy)
	{
		return false;
	}

	const UCombatCharacterComponent* TargetComp = TargetCharacterComp.Get();
	if (!TargetComp || TargetComp->GetTeam() != ECombatTeam::Player)
	{
		return false;
	}

	return true;
}

bool UTargetGuideLineComponent::IsActorDead(const AActor* Actor) const
{
	if (!Actor)
	{
		return true;
	}

	const UHPComponent* HP = nullptr;
	if (Actor == GetOwner())
	{
		HP = OwnerHPComp.Get();
	}
	else if (Actor == AggroTarget.Get())
	{
		HP = TargetHPComp.Get();
	}

	return HP && HP->IsDead();
}

FVector UTargetGuideLineComponent::ResolveOwnerGuideLocation() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	if (const USkeletalMeshComponent* Mesh = OwnerMeshComp.Get())
	{
		if (!StartSocketName.IsNone() && Mesh->DoesSocketExist(StartSocketName))
		{
			return Mesh->GetSocketLocation(StartSocketName);
		}
	}

	return Owner->GetActorLocation() + FVector(0.f, 0.f, OwnerFallbackHeightOffset);
}

FVector UTargetGuideLineComponent::ResolveTargetGuideLocation(const AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	if (const USkeletalMeshComponent* Mesh = TargetMeshComp.Get())
	{
		const FBoxSphereBounds Bounds = Mesh->Bounds;
		return Bounds.Origin;
	}

	return Target->GetActorLocation() + FVector(0.f, 0.f, TargetHeightOffset);
}

FLinearColor UTargetGuideLineComponent::ResolveGuideColor(const AActor* Target) const
{
	if (Target && Target == AggroTarget.Get())
	{
		if (const UCombatCharacterComponent* TargetComp = TargetCharacterComp.Get())
		{
			if (TargetComp->GetRole() == EJRPGPartyRole::Defender)
			{
				return DefenderTargetColor;
			}
		}
	}

	return NonDefenderTargetColor;
}
