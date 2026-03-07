#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExplorationDiscoveryVolume.generated.h"

class UBoxComponent;
class UExplorationRewardTableAsset;

UCLASS()
class JRPG_API AExplorationDiscoveryVolume : public AActor
{
	GENERATED_BODY()

public:
	AExplorationDiscoveryVolume();

	UPROPERTY(EditAnywhere)
	FName DiscoveryId = NAME_None;

	// 발견 보상(선택)
	UPROPERTY(EditAnywhere)
	TObjectPtr<UExplorationRewardTableAsset> OptionalDiscoveryReward = nullptr;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Box;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* Overlapped, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);
};
