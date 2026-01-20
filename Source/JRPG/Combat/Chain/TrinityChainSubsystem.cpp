#include "TrinityChainSubsystem.h"
#include "JRPG/Combat/Battle/BattleSessionSubsystem.h"
#include "JRPG/Combat/Time/CombatTimeSubsystem.h"
#include "JRPG/Combat/AI/CombatAIComponent.h"
#include "JRPG/Combat/Skill/SkillComponent.h"
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "JRPG/Combat/Stats/HealthComponent.h"
#include "Engine/World.h"

void UTrinityChainSubsystem::Tick(float DeltaTime)
{
    if (!GetWorld()) return;

    if (State == EChainState::Executing)
    {
        const double Now = GetWorld()->GetRealTimeSeconds();
        if (Now >= NextStepRealTime)
            ExecuteStep();
    }
}

TArray<AActor*> UTrinityChainSubsystem::GetPartyRaw() const
{
    TArray<AActor*> Out;
    for (const auto& P : PartyMembers) if (AActor* A = P.Get()) Out.Add(A);
    return Out;
}

void UTrinityChainSubsystem::EnterChainTimeStop()
{
    if (UCombatTimeSubsystem* Time = GetWorld()->GetSubsystem<UCombatTimeSubsystem>())
        Time->EnterChainStop(0.01f);
}

void UTrinityChainSubsystem::ExitChainTimeStop()
{
    if (UCombatTimeSubsystem* Time = GetWorld()->GetSubsystem<UCombatTimeSubsystem>())
        Time->ExitChainStop();
}

void UTrinityChainSubsystem::StopAllAI()
{
    if (!GetWorld()) return;
    if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
    {
        for (AActor* A : Battle->GetPartyRaw())
            if (UCombatAIComponent* AI = A ? A->FindComponentByClass<UCombatAIComponent>() : nullptr) AI->StopAI();

        for (AActor* E : Battle->GetEnemiesRaw())
            if (UCombatAIComponent* AI = E ? E->FindComponentByClass<UCombatAIComponent>() : nullptr) AI->StopAI();
    }
}

void UTrinityChainSubsystem::ResumeAllAI()
{
    if (!GetWorld()) return;
    if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
    {
        for (AActor* A : Battle->GetPartyRaw())
            if (UCombatAIComponent* AI = A ? A->FindComponentByClass<UCombatAIComponent>() : nullptr) AI->StartAI();

        for (AActor* E : Battle->GetEnemiesRaw())
            if (UCombatAIComponent* AI = E ? E->FindComponentByClass<UCombatAIComponent>() : nullptr) AI->StartAI();
    }
}

void UTrinityChainSubsystem::StartChain(AActor* Target, bool bAutoSelectAndExecute)
{
    if (!GetWorld() || State != EChainState::None) return;
    if (!Target) return;

    if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
    {
        if (!Battle->IsInBattle()) return;

        ChainTarget = Target;

        PartyMembers.Reset();
        SelectedSkills.Reset();
        ExecutionList.Reset();
        ExecIndex = 0;
        TP = 0;

        for (AActor* P : Battle->GetPartyRaw())
            if (P) PartyMembers.Add(P);

        StopAllAI();
        EnterChainTimeStop();

        State = EChainState::Selecting;
        OnChainSelecting.Broadcast();

        if (bAutoSelectAndExecute)
        {
            AutoSelect();
            ConfirmAndExecute();
        }
    }
}

void UTrinityChainSubsystem::AutoSelect()
{
    for (auto& P : PartyMembers)
    {
        AActor* A = P.Get();
        if (!A) continue;

        if (USkillComponent* Skills = A->FindComponentByClass<USkillComponent>())
        {
            // 슬롯1을 기본 추천(데이터로 바꾸는 건 이후)
            if (UCombatSkill* S = Skills->GetSkillBySlot(1))
                SelectedSkills.Add(A, S->SkillId);
        }
    }
}

void UTrinityChainSubsystem::SelectSkillFor(AActor* PartyMember, FName SkillId)
{
    if (State != EChainState::Selecting) return;
    if (!PartyMember || SkillId.IsNone()) return;
    SelectedSkills.Add(PartyMember, SkillId);
}

void UTrinityChainSubsystem::BuildExecutionList()
{
    ExecutionList.Reset();
    for (auto& P : PartyMembers)
    {
        AActor* A = P.Get();
        if (!A) continue;

        const FName* Id = SelectedSkills.Find(A);
        if (!Id || Id->IsNone()) continue;

        ExecutionList.Add(FExecEntry{A, *Id});
    }
}

void UTrinityChainSubsystem::ConfirmAndExecute()
{
    if (!GetWorld() || State != EChainState::Selecting) return;
    if (!ChainTarget.IsValid()) { CancelChain(); return; }

    BuildExecutionList();
    ExecIndex = 0;

    State = EChainState::Executing;
    OnChainExecuting.Broadcast();

    NextStepRealTime = GetWorld()->GetRealTimeSeconds(); // 즉시
}

void UTrinityChainSubsystem::ExecuteStep()
{
    if (!GetWorld()) return;

    AActor* Target = ChainTarget.Get();
    if (!Target)
    {
        CancelChain();
        return;
    }

    if (ExecIndex >= ExecutionList.Num())
    {
        // 피니시: TP 기반 추가 데미지(단순 버전)
        const float FinishDamage = 40.f + (float)TP * 10.f;

        if (UHealthComponent* HP = Target->FindComponentByClass<UHealthComponent>())
            HP->ApplyDamage(FinishDamage);

        // 종료
        State = EChainState::None;
        ExitChainTimeStop();
        ResumeAllAI();

        PartyMembers.Reset();
        SelectedSkills.Reset();
        ExecutionList.Reset();
        ChainTarget = nullptr;

        OnChainEnded.Broadcast();
        return;
    }

    const FExecEntry& Entry = ExecutionList[ExecIndex];
    AActor* User = Entry.User.Get();
    if (!User)
    {
        ExecIndex++;
        NextStepRealTime = GetWorld()->GetRealTimeSeconds() + StepDelay;
        return;
    }

    if (USkillComponent* Skills = User->FindComponentByClass<USkillComponent>())
    {
        if (UCombatSkill* Skill = Skills->FindSkillById(Entry.SkillId))
        {
            // 실행 성공하면 TP 누적 (APCost 기반)
            const bool bOk = Skills->TryExecuteSkill(Skill, Target);
            if (bOk) TP += FMath::Max(1, Skill->APCost);
            else
            {
                // 실패 시 기본공격으로 대체
                if (UCombatSkill* Basic = Skills->GetBasicAttack())
                {
                    if (Skills->TryExecuteSkill(Basic, Target))
                        TP += 1;
                }
            }
        }
    }

    ExecIndex++;
    NextStepRealTime = GetWorld()->GetRealTimeSeconds() + StepDelay;
}

void UTrinityChainSubsystem::CancelChain()
{
    if (!GetWorld()) return;
    if (State == EChainState::None) return;

    State = EChainState::None;
    ExitChainTimeStop();
    ResumeAllAI();

    PartyMembers.Reset();
    SelectedSkills.Reset();
    ExecutionList.Reset();
    ChainTarget = nullptr;

    OnChainEnded.Broadcast();
}
