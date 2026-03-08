// Source/JRPGCombat/Private/Combat/AI/CombatPartyAIComponent.cpp

#include "Combat/AI/CombatPartyAIComponent.h"
#include "Combat/AI/CombatAIContext.h"
#include "Combat/AI/CombatAIScorer.h"

#include "Combat/Skills/SkillComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

UCombatPartyAIComponent::UCombatPartyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;// 내부에서 DecisionInterval로 제어
}

void UCombatPartyAIComponent::BeginPlay()
{
	Super::BeginPlay();

	Context = NewObject<UCombatAIContext>(this);
	Context->Initialize(GetOwner(), Role,PresetAsset);

	Scorer = NewObject<UCombatAIScorer>(this);
	Scorer->Initialize(FGetSkillAIMetaDelegate::CreateUObject(
		this, &UCombatPartyAIComponent::ResolveSkillMeta
		));
}

void UCombatPartyAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UCombatPartyAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Context||!Scorer)
		return;

	// 조작 캐릭터는 플레이어 입력 우선 :contentReference[oaicite:33]{index=33}
	if (APawn *P =Cast<APawn>(GetOwner()))
	{
		if (AController *C = P->GetController())
		{
			if (C->IsPlayerController())
				return;
		}
	}

	RefreshContext();

	// Chain 시퀀스면 일반 AI 완전 억제
	if (Context->bInChainSequence)
	{
		State = EPartyAIState::SuppressedByChain;
		return;
	}

	const float Interval = (PresetAsset ?PresetAsset->DecisionIntervalSec :0.25f);
	DecisionAccum += DeltaTime;
	if (DecisionAccum < Interval)
		return;
	DecisionAccum = 0.f;

	UpdateStateMachine();
	const FCombatAIAction Best =ChooseBestAction();
	ExecuteAction(Best);
}

void UCombatPartyAIComponent::RefreshContext()
{
	Context->Role = Role;
	Context->PresetAsset = PresetAsset;

	// TODO: 프로젝트의 CombatParticipantRegistry 연동 시 PartyMembers / PrimaryTarget 채움.
	Context->Refresh();
}

void UCombatPartyAIComponent::UpdateStateMachine()
{
	if (!Context->bSessionActive || Context->bSelfIsDead)
	{
		State = EPartyAIState::Recover;
		return;
	}

	// 전술 예약 실행은 SkillComponent가 소비 (Tactical은 예약 데이터 제공만) :contentReference[oaicite:34]{index=34}
	// 여기서는 예약 존재 여부 훅만 남겨두고, 실제 예약 판단은 SkillComponent에 붙임.
	State = EPartyAIState::ExecuteRole;
}

FCombatAIAction UCombatPartyAIComponent::ChooseBestAction()const
{
	if (!Context->SkillComp.IsValid())
		return FCombatAIAction::MakeWait(0.f);

	TWeakObjectPtr<AActor>Target =Context->PrimaryTarget;

	// 후보: BasicAttack / OwnedSkills
	FCombatAIAction Best = FCombatAIAction::MakeWait(0.05f);

	{
		const float S =Scorer->ScoreAction(*Context, FCombatAIAction::MakeBasicAttack(Target,0.f));
		if (S > Best.Score)
			Best = FCombatAIAction::MakeBasicAttack(Target,S);
	}

	// 스킬 목록은 프로젝트의 SkillComponent API에 맞춰 연결
	TArray<FName>OwnedSkills;
	Context->SkillComp->GetOwnedSkillIds(OwnedSkills);// <- 너희 SkillComponent에 맞춰 제공하면 됨

	for (const FName SkillId :OwnedSkills)
	{
		if (SkillId.IsNone())
			continue;

		// AI는 스킬에 사용 요청만 한다(최종 권위는 SkillComponent) :contentReference[oaicite:35]{index=35}
		if (!Context->SkillComp->CanUseSkill(SkillId))
			continue;

		const FCombatAIAction A = FCombatAIAction::MakeUseSkill(SkillId,Target,0.f);
		const float S =Scorer->ScoreAction(*Context,A);
		if (S > Best.Score)
		{
			Best = A;
			Best.Score = S;
		}
	}

	return Best;
}

void UCombatPartyAIComponent::ExecuteAction(const FCombatAIAction &Action)
{
	if (!Context->SkillComp.IsValid())
		return;

	if (Action.Type == ECombatAIActionType::Wait)
		return;

	// 기본 공격도 스킬 요청으로 통합(스킬 문서: 단일 API) :contentReference[oaicite:36]{index=36}
	if (Action.Type == ECombatAIActionType::BasicAttack)
	{
		Context->SkillComp->RequestBasicAttack(Action.Target.Get());
		return;
	}

	if (Action.Type == ECombatAIActionType::UseSkill)
	{
		Context->SkillComp->RequestUseSkillByAI(Action.SkillId, Action.Target.Get());
		return;
	}
}

bool UCombatPartyAIComponent::ResolveSkillMeta(USkillComponent *SkillComp, FName SkillId, FSkillAIMeta &OutMeta) const
{
	OutMeta = FSkillAIMeta();

	if (!SkillComp || SkillId.IsNone())
		return false;

	// 프로젝트 SSOT: SkillDataAsset/SkillSpec에 효과 타입 태그 넣기
	// (Heal/Taunt/Cleanse/Break/Debuff/Buff 등)
	// 여기서는 예시용 API. 너희 SkillComponent에 맞춰 구현해주면 됨.
	return SkillComp->GetSkillAIMeta(SkillId,OutMeta);
}