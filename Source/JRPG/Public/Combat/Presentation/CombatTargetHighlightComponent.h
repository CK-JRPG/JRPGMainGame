#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "CombatTargetHighlightComponent.generated.h"

class UMeshComponent;

UENUM(BlueprintType)
enum class ECombatTargetHighlightMode : uint8
{
	None UMETA(DisplayName = "None"),
	SoftTarget UMETA(DisplayName = "Soft Target"),
	LockedOn UMETA(DisplayName = "Locked On")
};

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPG_API UCombatTargetHighlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatTargetHighlightComponent();

	UFUNCTION(BlueprintCallable, Category = "Combat|Target Highlight")
	void SetHighlightMode(ECombatTargetHighlightMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Combat|Target Highlight")
	void ClearHighlight();

	UFUNCTION(BlueprintPure, Category = "Combat|Target Highlight")
	ECombatTargetHighlightMode GetHighlightMode() const { return HighlightMode; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Target Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 SoftTargetStencilValue = 250;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Target Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 LockedOnStencilValue = 251;

private:
	struct FMeshCustomDepthState
	{
		TWeakObjectPtr<UMeshComponent> Mesh;
		bool bRenderCustomDepth = false;
		int32 StencilValue = 0;
		ERendererStencilMask StencilWriteMask = ERendererStencilMask::ERSM_Default;
	};

	ECombatTargetHighlightMode HighlightMode = ECombatTargetHighlightMode::None;
	TArray<FMeshCustomDepthState> CachedMeshStates;

	void ApplyHighlight();
	void CacheMeshStatesForCurrentOwner();
	void CacheMeshStateIfNeeded(UMeshComponent* Mesh);
	FMeshCustomDepthState* FindCachedState(UMeshComponent* Mesh);
	int32 GetStencilValueForMode(ECombatTargetHighlightMode Mode) const;
	bool IsHighlightableMesh(const UMeshComponent* Mesh) const;
};
