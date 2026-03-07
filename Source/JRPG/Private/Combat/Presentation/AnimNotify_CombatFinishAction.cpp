#include "Combat/Presentation/AnimNotify_CombatFinishAction.h"
#include "Combat/Presentation/CombatPresentationComponent.h"

void UAnimNotify_CombatFinishAction::Notify(USkeletalMeshComponent *MeshComp,UAnimSequenceBase*)
{
	if (!MeshComp)
		return;
	
	if (AActor *Owner = MeshComp->GetOwner())
	{
		if (UCombatPresentationComponent*P = Owner->FindComponentByClass<UCombatPresentationComponent>())
		{
			P->FinishActivePresentation();
		}
	}
}