// Source/JRPGCombat/Public/Combat/AI/CombatPartyAIComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/AI/CombatAIPresetAsset.h"
#include "Combat/AI/CombatAIActionTypes.h"
#include "CombatPartyAIComponent.generated.h"

class UCombatAIContext;
class UCombatAIScorer;
class UJRPGSkillComponent;

UCLASS(ClassGroup=(JRPGCombat),meta=(BlueprintSpawnableComponent=false))
class JRPG_API UCombatPartyAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatPartyAIComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 매 프레임이 아니라 “결정 주기(DecisionInterval)”로 갱신
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	// 역할/프리셋은 코드에서 지정(블루프린트 최소화)
	UPROPERTY(EditAnywhere) EPartyRole Role = EPartyRole::Attacker;
	UPROPERTY(EditAnywhere) TObjectPtr<UCombatAIPresetAsset> PresetAsset;

	// 디버그용
	UPROPERTY(VisibleAnywhere) EPartyAIState State = EPartyAIState::Follow;

private:
	UPROPERTY() TObjectPtr<UCombatAIContext> Context;
	UPROPERTY() TObjectPtr<UCombatAIScorer> Scorer;

	UPROPERTY() float DecisionAccum = 0.f;

	void RefreshContext();
	void UpdateStateMachine();
	FCombatAIAction ChooseBestAction() const;
	void ExecuteAction(const FCombatAIAction &Action);

	bool ResolveSkillMeta(UJRPGSkillComponent *SkillComp, FName SkillId,/*out*/struct FSkillAIMeta &OutMeta) const;
};