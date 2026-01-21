#include "BattleSessionSubsystem.h"
#include "BattleZoneActor.h"

#include "JRPG/Combat/Stats/APComponent.h"
#include "JRPG/Combat/Stats/HealthComponent.h"
#include "JRPG/Combat/AI/CombatAIComponent.h"
#include "JRPG/Combat/Threat/ThreatComponent.h"

#include "TimerManager.h"
#include "Engine/World.h"

TArray<AActor*> UBattleSessionSubsystem::GetEnemiesRaw() const
{
    TArray<AActor*> Out;
    for (const auto& W : EnemyActors) if (AActor* A = W.Get()) Out.Add(A);
    return Out;
}

TArray<AActor*> UBattleSessionSubsystem::GetPartyRaw() const
{
    TArray<AActor*> Out;
    for (const auto& W : PartyMembers) if (AActor* A = W.Get()) Out.Add(A);
    return Out;
}

void UBattleSessionSubsystem::StartBattle(
    const FVector& Center,
    float RadiusMeters,
    const TArray<AActor*>& Party,
    const TArray<AActor*>& Enemies,
    AActor* InitialAggroTarget
)
{
    //Log
    UE_LOG(LogTemp, Warning, TEXT(">>> [StartBattle]"));
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT(">>> Battle STARTED!"));

    if (bInBattle || !GetWorld()) return;

    bInBattle = true;
    BattleCenter = Center;
    BattleRadiusCm = RadiusMeters * 100.f;
    InitialAggro = InitialAggroTarget;

    PartyMembers.Reset();
    EnemyActors.Reset();

    for (AActor* A : Party)   if (A) PartyMembers.Add(A);
    for (AActor* A : Enemies) if (A) EnemyActors.Add(A);

    MainEnemy = (Enemies.Num() > 0) ? Enemies[0] : nullptr;

    Zone = GetWorld()->SpawnActor<ABattleZoneActor>();
    if (Zone)
    {
        Zone->SetCenter(BattleCenter);
        Zone->Radius = BattleRadiusCm;
    }

    SetupParticipants();

    GetWorld()->GetTimerManager().SetTimer(TargetedUpdateTimer, this, &UBattleSessionSubsystem::UpdateTargetedFlags, 0.2f, true);
    GetWorld()->GetTimerManager().SetTimer(EndCheckTimer, this, &UBattleSessionSubsystem::CheckEndCondition, 0.5f, true);

    OnBattleStarted.Broadcast();
}

void UBattleSessionSubsystem::SetupParticipants()
{
    const TArray<AActor*> Party = GetPartyRaw();
    const TArray<AActor*> Enemies = GetEnemiesRaw();

    // 전투 구역 등록 + 파티 AP 리젠/AI 시작
    for (AActor* P : Party)
    {
        if (!P) continue;

        if (Zone) Zone->RegisterParticipant(P);

        if (UAPComponent* AP = P->FindComponentByClass<UAPComponent>())
            AP->StartRegen();

        if (UCombatAIComponent* AI = P->FindComponentByClass<UCombatAIComponent>())
        {
            AI->SetMainTarget(MainEnemy.Get());
            AI->StartAI();
        }
    }

    // 적: Threat 초기화(파티) + 초기 어그로 + 적 AI 시작
    for (AActor* E : Enemies)
    {
        if (!E) continue;

        if (Zone) Zone->RegisterParticipant(E);

        if (UThreatComponent* Threat = E->FindComponentByClass<UThreatComponent>())
        {
            Threat->InitThreat(Party);

            // 초기 어그로: 전투 트리거된 플레이어를 잠깐 고정
            if (AActor* Aggro = InitialAggro.Get())
            {
                Threat->AddThreat(Aggro, 1000.f);
                Threat->ForceTarget(Aggro, 2.0f);
            }
        }

        if (UCombatAIComponent* EnemyAI = E->FindComponentByClass<UCombatAIComponent>())
        {
            // 적은 Threat의 현재 타겟을 따라가게
            if (UThreatComponent* Threat = E->FindComponentByClass<UThreatComponent>())
                EnemyAI->SetMainTarget(Threat->GetCurrentTarget());

            EnemyAI->StartAI();
        }
    }
}

void UBattleSessionSubsystem::UpdateTargetedFlags()
{
    const TArray<AActor*> Party = GetPartyRaw();
    const TArray<AActor*> Enemies = GetEnemiesRaw();

    // 누가 타겟 당하고 있는지 계산
    TSet<AActor*> TargetedParty;
    for (AActor* E : Enemies)
    {
        if (!E) continue;
        if (UThreatComponent* Threat = E->FindComponentByClass<UThreatComponent>())
        {
            if (AActor* T = Threat->GetCurrentTarget())
                TargetedParty.Add(T);
        }
    }

    // Party AI에 타겟팅 플래그 반영
    for (AActor* P : Party)
    {
        if (!P) continue;
        if (UCombatAIComponent* AI = P->FindComponentByClass<UCombatAIComponent>())
            AI->SetIsTargeted(TargetedParty.Contains(P));
    }

    // 적 AI는 Threat 타겟을 계속 추종
    for (AActor* E : Enemies)
    {
        if (!E) continue;
        if (UCombatAIComponent* AI = E->FindComponentByClass<UCombatAIComponent>())
        {
            if (UThreatComponent* Threat = E->FindComponentByClass<UThreatComponent>())
                AI->SetMainTarget(Threat->GetCurrentTarget());
        }
    }
}

void UBattleSessionSubsystem::CheckEndCondition()
{
    if (!bInBattle) return;

    const TArray<AActor*> Party = GetPartyRaw();
    const TArray<AActor*> Enemies = GetEnemiesRaw();

    bool bAllPartyDead = true;
    for (AActor* P : Party)
    {
        if (!P) continue;
        if (UHealthComponent* HP = P->FindComponentByClass<UHealthComponent>())
        {
            if (!HP->IsDead()) { bAllPartyDead = false; break; }
        }
        else { bAllPartyDead = false; break; }
    }

    bool bAllEnemiesDead = true;
    for (AActor* E : Enemies)
    {
        if (!E) continue;
        if (UHealthComponent* HP = E->FindComponentByClass<UHealthComponent>())
        {
            if (!HP->IsDead()) { bAllEnemiesDead = false; break; }
        }
        else { bAllEnemiesDead = false; break; }
    }

    if (bAllEnemiesDead) EndBattle(EBattleResult::Victory);
    else if (bAllPartyDead) EndBattle(EBattleResult::Defeat);
}

void UBattleSessionSubsystem::EndBattle(EBattleResult Result)
{
    if (!bInBattle || !GetWorld()) return;

    //Log
    FString ResultStr = (Result == EBattleResult::Victory) ? TEXT("VICTORY") : TEXT("DEFEAT");
    UE_LOG(LogTemp, Warning, TEXT(">>> [EndBattle] Result: %s"), *ResultStr);
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT(">>> Battle ENDED: %s"), *ResultStr));

    bInBattle = false;

    GetWorld()->GetTimerManager().ClearTimer(TargetedUpdateTimer);
    GetWorld()->GetTimerManager().ClearTimer(EndCheckTimer);

    // 파티 AP/AI 종료
    for (AActor* P : GetPartyRaw())
    {
        if (!P) continue;

        if (UAPComponent* AP = P->FindComponentByClass<UAPComponent>())
            AP->StopRegen();

        if (UCombatAIComponent* AI = P->FindComponentByClass<UCombatAIComponent>())
            AI->StopAI();
    }

    // 적 AI 종료
    for (AActor* E : GetEnemiesRaw())
    {
        if (!E) continue;
        if (UCombatAIComponent* AI = E->FindComponentByClass<UCombatAIComponent>())
            AI->StopAI();
    }

    if (Zone)
    {
        Zone->Destroy();
        Zone = nullptr;
    }

    PartyMembers.Reset();
    EnemyActors.Reset();
    MainEnemy = nullptr;
    InitialAggro = nullptr;

    OnBattleEnded.Broadcast(Result);
}