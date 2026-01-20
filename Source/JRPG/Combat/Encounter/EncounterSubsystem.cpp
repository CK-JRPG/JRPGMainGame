#include "EncounterSubsystem.h"
#include "JRPG/Combat/Battle/BattleSessionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UEncounterSubsystem::CollectActors(const FVector& CenterCm, float RadiusCm, TArray<AActor*>& OutParty, TArray<AActor*>& OutEnemies) const
{
	OutParty.Reset();
	OutEnemies.Reset();

	TArray<AActor*> PartyAll;
	TArray<AActor*> EnemyAll;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "PartyMember", PartyAll);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Enemy", EnemyAll);

	for (AActor* P : PartyAll)
	{
		if (!P) continue;
		OutParty.Add(P); // 파티는 항상 참여(원하면 거리 필터 추가 가능)
	}

	for (AActor* E : EnemyAll)
	{
		if (!E) continue;
		const float Dist = FVector::Dist2D(E->GetActorLocation(), CenterCm);
		if (Dist <= RadiusCm)
			OutEnemies.Add(E);
	}
}

void UEncounterSubsystem::RequestEncounter(AActor* TriggerEnemy, AActor* TriggerActor, EEncounterTrigger Trigger)
{
	if (!GetWorld()) return;

	if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		if (Battle->IsInBattle()) return;

		// 전투 중심은 플레이어 기준(요구사항)
		const FVector Center = TriggerActor ? TriggerActor->GetActorLocation() : (TriggerEnemy ? TriggerEnemy->GetActorLocation() : FVector::ZeroVector);
		const float RadiusCm = EncounterRadiusMeters * 100.f;

		TArray<AActor*> Party;
		TArray<AActor*> Enemies;
		CollectActors(Center, RadiusCm, Party, Enemies);

		// TriggerEnemy를 메인 타겟으로 맨 앞에 배치
		if (TriggerEnemy)
		{
			Enemies.Remove(TriggerEnemy);
			Enemies.Insert(TriggerEnemy, 0);
		}

		if (Party.Num() == 0 || Enemies.Num() == 0) return;

		Battle->StartBattle(Center, EncounterRadiusMeters, Party, Enemies, TriggerActor);
	}
}
