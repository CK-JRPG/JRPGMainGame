// Source/JRPGCombat/Private/Combat/AI/PartyAIComponent.cpp
#include "Combat/AI/PartyAIComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

#include "Combat/AI/CombatAIContext.h"
#include "Combat/AI/CombatAIScorer.h"
#include "Combat/AI/CombatAIPresetAsset.h"

#include "Combat/Skills/SkillComponent.h"
#include "JRPGCombat/Public/Combat/Infrastructure/CombatBattleSessionSubsystem.h"

UPartyAIComponent::UPartyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPartyAIComponent::BeginPlay()
{
	Super::BeginPlay();
StartLoop();
}

void UPartyAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLoop();
	Super::EndPlay(EndPlayReason);
}

void UPartyAIComponent::SetPreset(ECombatAIPreset NewPreset)
{
	Preset = NewPreset;
}

void UPartyAIComponent::SetRole(ECombatPartyRole NewRole)
{
	Role = NewRole;
}

bool UPartyAIComponent::IsPlayerControlledNow()const
{
	if (!bDisableWhenPlayerControlled) return false;

	const APawn*P = Cast<APawn>(GetOwner());
	if (!P) return false;

	const AController *C = P->GetController();
	return (C && C->IsPlayerController());
}

void UPartyAIComponent::StartLoop()
{
	if (!GetWorld()) return;

	// 프리셋 없으면 루프 돌지 않게(크래시 방지)
	if (!PresetAsset)
	{
		return;
	}

	const float Interval = FMath::Max(0.05f, PresetAsset->DecisionIntervalSec);
	GetWorld()->GetTimerManager().SetTimer(DecisionTimer,this, &UPartyAIComponent::ThinkOnce,Interval,true);
}

void UPartyAIComponent::StopLoop()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(DecisionTimer);
}

void UPartyAIComponent::ThinkOnce()
{
	if (!GetOwner()||!PresetAsset)return;

	// 플레이어 조작 중이면 파티 AI는 꺼짐
	if (IsPlayerControlledNow())return;

	UCombatAIContext *CtxObj = UCombatAIContext::Build(GetOwner());
	if (!CtxObj) return;

	// 체인 동안 멈춤(체인은 별도 시퀀스 전투처럼 보이게)
	if (bPauseDuringChain && CtxObj->bChainActive) return;

	const FJRPGCombatAIAction Action = FCombatAIScorer::ChoosePartyAction(*CtxObj, *PresetAsset, Role, Preset);

	USkillComponent*Skill = CtxObj->Skill;
	if (!Skill) return;

	switch (Action.Type)
	{
	case EJRPGCombatAIActionType::UseSkill:
		{
			Skill->RequestUseSkill(Action.SkillId,Action.Target.Get(),/*Source*/ESkillRequestSource::AI);
			break;
		}
	case EJRPGCombatAIActionType::BasicAttack:
		{
			Skill->RequestBasicAttack(Action.Target.Get(),/*Source*/ESkillRequestSource::AI);
			break;
		}
	default:
		break;
	}
}