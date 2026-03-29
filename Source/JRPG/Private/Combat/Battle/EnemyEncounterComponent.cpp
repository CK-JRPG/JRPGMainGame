
#include "Combat/Battle/EnemyEncounterComponent.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/SphereComponent.h"
#include "Game/JRPGPlayerPawn.h"

UEnemyEncounterComponent::UEnemyEncounterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UEnemyEncounterComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (UCombatCharacterComponent* CombatCharComp = GetOwner()->FindComponentByClass<UCombatCharacterComponent>())
	{
		//팀 타입이 Enemy일 때만 해당 컴포넌트 사용가능.
		if (CombatCharComp->GetTeam() != ECombatTeam::Enemy)
		{
			SetComponentTickEnabled(false);
			return;
		}
	}
	else
	{
		return;
	}
	
	TriggerSphere = NewObject<USphereComponent>(GetOwner(), TEXT("EncounterTrigger"));
	TriggerSphere->SetupAttachment(GetOwner()->GetRootComponent());
	TriggerSphere->RegisterComponent();

	TriggerSphere->SetSphereRadius(DetectionRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &UEnemyEncounterComponent::OnTriggerOverlap);
}

void UEnemyEncounterComponent::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasTriggered) return;

	// 이미 전투 중이면 무시
	if (UBattleSessionSubsystem* BattleSub = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		if (BattleSub->IsBattleActive()) return;
	}

	AJRPGPlayerPawn* PlayerPawn = Cast<AJRPGPlayerPawn>(OtherActor);
	if (!PlayerPawn) return;

	bHasTriggered = true;
	SearchCombatCharactersInRadius(OtherActor);
}

void UEnemyEncounterComponent::SearchCombatCharactersInRadius(const AActor* PlayerActor)
{
	FBattleSessionConfig BattleConfig;
	TSet<const AActor*> AddedActors;
	AddedActors.Add(PlayerActor); 

	// 현재 자기 자신(인카운터 된 적)도 EnemySide에 추가
	AActor* Owner = GetOwner();
	if (ICombatParticipantInterface* Self = Cast<ICombatParticipantInterface>(Owner))
	{
		if (Self->GetHP() && !Self->GetHP()->IsDead())
		{
			BattleConfig.EnemySide.Add(Owner);
			AddedActors.Add(Owner);
		}
	}

}

void UEnemyEncounterComponent::ReadyForBattleSession(const FBattleSessionConfig& Config)
{
}

void UEnemyEncounterComponent::CreateCombatZone()
{
}
