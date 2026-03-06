// Source/JRPGCombat/Public/Combat/Exploration/ExplorationDiscoveryVolume.h
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

	// 발견 보상(선택): 문서 “필요 시 소량의 보상” :contentReference[oaicite:38]{index=38}
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
