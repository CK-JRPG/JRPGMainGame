
#include "Combat/Battle/EnemyEncounterComponent.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
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
	SearchCombatEnemyCharactersInRadius(OtherActor);
}

void UEnemyEncounterComponent::SearchCombatEnemyCharactersInRadius(const AActor* PlayerActor)
{
	if (!IsValid(PlayerActor))
		return; 
	
	UWorld* World = GetWorld();
	UGameInstance* GI = GetOwner()->GetGameInstance();
	if (!World || !GI)
	{
		bHasTriggered = false;
		return;
	}
	
	UPartySubsystem* PartySys = GI->GetSubsystem<UPartySubsystem>();
	UPartyActorSpawnSubsystem* SpawnSub = World->GetSubsystem<UPartyActorSpawnSubsystem>();
	UPhysicsFieldComponent* CompanionSub = World->GetSubsystem<UPhysicsFieldComponent>();
	
	if (!PartySys || !SpawnSub || !CompanionSub)
	{
		bHasTriggered = false;
		return;
	}
	
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
	
	const FVector TriggerLocation = GetOwner()->GetActorLocation();
	FCollisionShape Sphere = FCollisionShape::MakeSphere(EnemySearchRadius);
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);
	
	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByObjectType(OverlapResults, TriggerLocation, FQuat::Identity, QueryParams,Sphere);
	
	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!IsValid(Candidate) || AddedActors.Contains(Candidate))
			continue;
		
		if (ICombatParticipantInterface* Participant = Cast<ICombatParticipantInterface>(Candidate))
		{
			if (Participant->GetCombatTeam() == ECombatTeam::Enemy && Participant->GetHP() && !Participant->GetHP()->IsDead())
			{
				BattleConfig.EnemySide.Add(Candidate);
				AddedActors.Add(Candidate);
			}
		}
	}
	
	if (BattleConfig.EnemySide.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterComp : 주변에 적이 없음."))
		bHasTriggered = false;
		return;
	}
	
	SpawnCombatPartyCharacters(PartySys, SpawnSub, BattleConfig);

}

void UEnemyEncounterComponent::SpawnCombatPartyCharacters(UPartySubsystem* PartySys,
	UPartyActorSpawnSubsystem* SpawnSub, FBattleSessionConfig& BattleConfig)
{
	const TArray<FName>& PartyIds = PartySys->GetPartyIds();
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 파티 멤버가 없음"));
		bHasTriggered = false;
		return;
	}
	
	TWeakObjectPtr<UEnemyEncounterComponent> WeakThis(this);
	TWeakObjectPtr<APlayerController> WeakPC;
	if (const APawn* Pawn = Cast<APawn>(Overlap))
	
}


void UEnemyEncounterComponent::ReadyForBattleSession(const FBattleSessionConfig& Config)
{
}

void UEnemyEncounterComponent::CreateCombatZone()
{
}
