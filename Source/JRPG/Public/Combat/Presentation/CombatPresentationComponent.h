#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/AnimMontage.h"

#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Combat/Skills/SkillTypes.h"
#include "Combat/Items/CombatItemTypes.h"

#include "CombatPresentationComponent.generated.h"

class USkillComponent;
class UCombatCharacterComponent;
class UTacticalModeSubsystem;
class UBattleSessionSubsystem;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatPresentationComponent();

	FOnCombatCueEvent OnCombatCueEvent;
	FOnPresentationStarted OnPresentationStarted;
	FOnPresentationFinished OnPresentationFinished;

	bool HasActivePresentation()const { return Active.Type!= EPresentedCombatActionType::None; }

	FCombatActionResult TryPresentBasicAttack(AActor*Target);
	FSkillCastResult TryPresentSkill(FName SkillId, const TArray<AActor*> &Targets,bool bFromTacticalReservation = false);
	FCombatItemUseResult TryPresentItem(FName ItemId, const TArray<AActor*> &Targets);

	void ResolveActivePresentation();
	void FinishActivePresentation();
	void CancelActivePresentation(FName ReasonTag,bool bRefundPreparedSkill);

	void EmitCue(FName CueTag);

protected:
	virtual void BeginPlay()override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
	struct FActivePresentationState
	{
		EPresentedCombatActionType Type = EPresentedCombatActionType::None;
		FName ActionId = NAME_None;
		TArray<TWeakObjectPtr<AActor>> Targets;
		bool bResolved = false;
		bool bFromTacticalReservation = false;

		ECombatResolveTiming ResolveTiming = ECombatResolveTiming::Immediate;
		TObjectPtr<UAnimMontage> Montage = nullptr;

		FName StartCueTag = NAME_None;
		FName HitCueTag = NAME_None;
		FName FinishCueTag = NAME_None;
	};

	FActivePresentationState Active;

	TWeakObjectPtr<USkillComponent> SkillComp;
	TWeakObjectPtr<UCombatCharacterComponent> CharacterComp;

	UBattleSessionSubsystem* GetBattle()const;
	UTacticalModeSubsystem* GetTactical()const;

	void PlayActiveMontageOrResolve();
	void ClearActiveState();

	void TryConsumeTacticalReservation();
};