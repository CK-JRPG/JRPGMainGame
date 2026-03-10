#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CombatResolveWindow.generated.h"

UCLASS()
class JRPG_API UAnimNotify_CombatResolveWindow : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation) override;
};