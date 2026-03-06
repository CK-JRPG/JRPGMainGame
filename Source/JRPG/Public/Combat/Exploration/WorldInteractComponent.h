// Source/JRPGCombat/Public/Combat/Exploration/WorldInteractComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Exploration/ExplorationTypes.h"
#include "WorldInteractComponent.generated.h"

class UExplorationSubsystem;
class AExplorationObjectActor;

UCLASS(ClassGroup = (Exploration), meta = (BlueprintSpawnableComponent))
class JRPG_API UWorldInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWorldInteractComponent();

	// 스캔 주기(틱 부담 줄이기)
	UPROPERTY(EditAnywhere)
	float ScanIntervalSec = 0.10f;

	// LOS trace channel
	UPROPERTY(EditAnywhere)
	TEnumAsByte<ECollisionChannel> LOSTraceChannel = ECC_Visibility;

	// 현재 프롬프트 대상
	UPROPERTY(VisibleAnywhere)
	FGuid CurrentObjectId;

	// 외부 입력 바인딩에서 호출
	UFUNCTION()
	void TryInteractInput();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle ScanTimer;

	void ScanOnce();

	bool HasLOSToActor(const AActor* Target) const;

	UExplorationSubsystem* GetExplore() const;
};
