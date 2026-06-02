#include "Combat/Presentation/AnimNotify_AttackEnd.h"

#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_AttackEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase*)
{
	if (!MeshComp)
	{
		return;
	}

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UCombatPresentationComponent* Presentation = Owner->FindComponentByClass<UCombatPresentationComponent>())
		{
			Presentation->FinishActivePresentation();
		}
	}
}