#include "Combat/Presentation/AnimNotify_CombatResolveWindow.h"
#include "Combat/Presentation/CombatPresentationComponent.h"

void UAnimNotify_CombatResolveWindow::Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase*)
{
	if (!MeshComp)
		return;
	
	if (AActor *Owner = MeshComp->GetOwner())
	{
		if (UCombatPresentationComponent *P = Owner->FindComponentByClass<UCombatPresentationComponent>())
		{
			P->ResolveActivePresentation();
		}
	}
}