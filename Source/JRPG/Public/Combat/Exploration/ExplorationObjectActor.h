// Source/JRPGCombat/Public/Combat/Exploration/ExplorationObjectActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExplorationObjectActor.generated.h"

class UExplorationObjectDataAsset;

UCLASS()
class JRPG_API AExplorationObjectActor : public AActor
{
	GENERATED_BODY()

public:
	AExplorationObjectActor();

	UPROPERTY(EditAnywhere)
	TObjectPtr<UExplorationObjectDataAsset> ObjectData = nullptr;

	const UExplorationObjectDataAsset* GetObjectData() const { return ObjectData; }

	// Subsystem이 활성/비활성(완료/리스폰 대기) 반영할 때 호출
	void SetExplorationActive(bool bActive);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool bIsActive = true;
};
