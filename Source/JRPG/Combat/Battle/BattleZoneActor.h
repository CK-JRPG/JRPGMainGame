#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleZoneActor.generated.h"

class UDecalComponent;
class UMaterialInterface;
class USceneComponent;

UCLASS()
class JRPG_API ABattleZoneActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleZoneActor();

	UPROPERTY(EditAnywhere, Category="Zone") float Radius = 10000.f; // cm (100m)
	UPROPERTY(EditAnywhere, Category="Zone") float SafetyMargin = 50.f;

	UPROPERTY(EditAnywhere, Category="Zone|Visual") bool bDrawDebugCircle = true;
	UPROPERTY(EditAnywhere, Category="Zone|Visual") float DebugCircleThickness = 3.f;

	UPROPERTY(EditAnywhere, Category="Zone|Visual") TObjectPtr<UMaterialInterface> ZoneDecalMaterial = nullptr;

	UFUNCTION() void SetCenter(const FVector& InCenter) { Center = InCenter; }
	UFUNCTION() FVector GetCenter() const { return Center; }

	UFUNCTION() void RegisterParticipant(AActor* Actor);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root = nullptr;
	UPROPERTY() TObjectPtr<UDecalComponent> ZoneDecal = nullptr;

	FVector Center = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AActor>> Participants;

	void ClampActor(AActor* Actor);
	void UpdateVisual();
};

