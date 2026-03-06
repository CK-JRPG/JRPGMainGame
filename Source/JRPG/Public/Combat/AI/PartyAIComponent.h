// Source/JRPGCombat/Public/Combat/AI/PartyAIComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/AI/CombatAITypes.h"
#include "PartyAIComponent.generated.h"

class UCombatAIPresetAsset;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UPartyAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPartyAIComponent();

	UPROPERTY(EditAnywhere,Category = "AI") ECombatPartyRole Role = ECombatPartyRole::Attacker;
	UPROPERTY(EditAnywhere,Category = "AI") ECombatAIPreset Preset = ECombatAIPreset::Basic;

	// 공용 프리셋(프로젝트 1개) 또는 캐릭터별로 갈아끼우는 방식 둘 다 가능
	UPROPERTY(EditAnywhere,Category="AI") TObjectPtr<UCombatAIPresetAsset> PresetAsset = nullptr;

	// 체인(제노블3처럼 별도 시퀀스) 동안 파티 AI는 보통 멈추는 게 안전
	UPROPERTY(EditAnywhere,Category="AI") bool bPauseDuringChain = true;

	// Possess(플레이어 조작) 중이면 AI는 자동 OFF
	UPROPERTY(EditAnywhere,Category="AI") bool bDisableWhenPlayerControlled = true;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetPreset(ECombatAIPreset NewPreset);
	void SetRole(ECombatPartyRole NewRole);

private:
	FTimerHandle DecisionTimer;

	void StartLoop();
	void StopLoop();
	void ThinkOnce();

	bool IsPlayerControlledNow() const;
};