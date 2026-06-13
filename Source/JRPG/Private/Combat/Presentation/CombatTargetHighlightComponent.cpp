#include "Combat/Presentation/CombatTargetHighlightComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"

UCombatTargetHighlightComponent::UCombatTargetHighlightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatTargetHighlightComponent::SetHighlightMode(ECombatTargetHighlightMode NewMode)
{
	if (NewMode == ECombatTargetHighlightMode::None)
	{
		ClearHighlight();
		return;
	}

	if (HighlightMode == ECombatTargetHighlightMode::None)
	{
		CacheMeshStatesForCurrentOwner();
	}

	HighlightMode = NewMode;
	ApplyHighlight();
}

void UCombatTargetHighlightComponent::ClearHighlight()
{
	if (HighlightMode == ECombatTargetHighlightMode::None && CachedMeshStates.Num() == 0)
	{
		return;
	}

	for (const FMeshCustomDepthState& State : CachedMeshStates)
	{
		if (UMeshComponent* Mesh = State.Mesh.Get())
		{
			Mesh->SetRenderCustomDepth(State.bRenderCustomDepth);
			Mesh->SetCustomDepthStencilWriteMask(State.StencilWriteMask);
			Mesh->SetCustomDepthStencilValue(State.StencilValue);
		}
	}

	CachedMeshStates.Empty();
	HighlightMode = ECombatTargetHighlightMode::None;
}

void UCombatTargetHighlightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearHighlight();
	Super::EndPlay(EndPlayReason);
}

void UCombatTargetHighlightComponent::ApplyHighlight()
{
	if (HighlightMode == ECombatTargetHighlightMode::None)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const int32 StencilValue = GetStencilValueForMode(HighlightMode);

	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents<UMeshComponent>(Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsHighlightableMesh(Mesh))
		{
			continue;
		}

		CacheMeshStateIfNeeded(Mesh);

		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
		Mesh->SetCustomDepthStencilValue(StencilValue);
	}
}

void UCombatTargetHighlightComponent::CacheMeshStatesForCurrentOwner()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents<UMeshComponent>(Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (IsHighlightableMesh(Mesh))
		{
			CacheMeshStateIfNeeded(Mesh);
		}
	}
}

void UCombatTargetHighlightComponent::CacheMeshStateIfNeeded(UMeshComponent* Mesh)
{
	if (!Mesh || FindCachedState(Mesh))
	{
		return;
	}

	FMeshCustomDepthState State;
	State.Mesh = Mesh;
	State.bRenderCustomDepth = Mesh->bRenderCustomDepth;
	State.StencilValue = Mesh->CustomDepthStencilValue;
	State.StencilWriteMask = Mesh->CustomDepthStencilWriteMask;
	CachedMeshStates.Add(State);
}

UCombatTargetHighlightComponent::FMeshCustomDepthState* UCombatTargetHighlightComponent::FindCachedState(UMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return nullptr;
	}

	return CachedMeshStates.FindByPredicate(
		[Mesh](const FMeshCustomDepthState& State)
		{
			return State.Mesh.Get() == Mesh;
		});
}

int32 UCombatTargetHighlightComponent::GetStencilValueForMode(ECombatTargetHighlightMode Mode) const
{
	switch (Mode)
	{
	case ECombatTargetHighlightMode::LockedOn:
		return FMath::Clamp(LockedOnStencilValue, 0, 255);
	case ECombatTargetHighlightMode::SoftTarget:
		return FMath::Clamp(SoftTargetStencilValue, 0, 255);
	case ECombatTargetHighlightMode::None:
	default:
		return 0;
	}
}

bool UCombatTargetHighlightComponent::IsHighlightableMesh(const UMeshComponent* Mesh) const
{
	if (!IsValid(Mesh) || Mesh->IsA<UWidgetComponent>())
	{
		return false;
	}

	return Mesh->IsA<USkeletalMeshComponent>() || Mesh->IsA<UStaticMeshComponent>();
}
