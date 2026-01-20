#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EncounterComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JRPG_API UEncounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 스킬/외부에서 호출 가능(옵션)
	UFUNCTION() void NotifyHitBy(AActor* InstigatorActor);
};
