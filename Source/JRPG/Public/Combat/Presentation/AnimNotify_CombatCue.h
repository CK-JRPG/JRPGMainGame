#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CombatCue.generated.h"

UCLASS()
class JRPG_API UAnimNotify_CombatCue : public UAnimNotify
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)FName CueTag = NAME_None;
	virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation) override;
};