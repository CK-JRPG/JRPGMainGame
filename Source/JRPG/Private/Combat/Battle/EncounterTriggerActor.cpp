// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Battle/EncounterTriggerActor.h"

#include "EngineUtils.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Session/CombatZoneActor.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "Game/JRPGPlayerPawn.h"

// Sets default values
AEncounterTriggerActor::AEncounterTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	TriggerVolume->SetCollisionObjectType(ECC_WorldStatic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	TriggerVolume->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
	
	//디버깅(나중엔 지울것)
	TriggerVolume->SetHiddenInGame(false); 
	TriggerVolume->ShapeColor = FColor::Red;
}

// Called when the game starts or when spawned
void AEncounterTriggerActor::BeginPlay()
{
	Super::BeginPlay();
	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AEncounterTriggerActor::OnOverlapBegin);
	}
}



void AEncounterTriggerActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasTriggered || !OtherActor)
		return;
	
	AJRPGPlayerPawn* PlayerPawn = Cast<AJRPGPlayerPawn>(OtherActor);
	if (!PlayerPawn)
		return;
	
	bHasTriggered = true;
	SearchCombatCharactersInRadius(OtherActor);
}

void AEncounterTriggerActor::SearchCombatCharactersInRadius(const AActor* OverlapActor)
{
	if (!IsValid(OverlapActor))
		return;

	UWorld* World = GetWorld();
	UGameInstance* GI = GetGameInstance();
	if (!World || !GI)
	{
		bHasTriggered = false;
		return;
	}

	UPartySubsystem* PartySys = GI->GetSubsystem<UPartySubsystem>();
	UPartyActorSpawnSubsystem* SpawnSub = World->GetSubsystem<UPartyActorSpawnSubsystem>();

	if (!PartySys || !SpawnSub)
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : Party 또는 PartyActorSpawn 서브시스템이 없음"));
		bHasTriggered = false;
		return;
	}

	// ── 1. 적 감지 (월드에 이미 배치된 CombatCharacterActor) ──
	FBattleSessionConfig BattleConfig;
	TSet<AActor*> AddedActors;
	AddedActors.Add(const_cast<AActor*>(OverlapActor));   // 플레이어 Pawn 제외

	const FVector TriggerLocation = GetActorLocation();
	const float SearchRadius = 1000.f;

	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByObjectType(OverlapResults, TriggerLocation, FQuat::Identity, QueryParams, Sphere);

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
				UE_LOG(LogTemp, Log, TEXT("EncounterTrigger : 적 캐릭터 %s 추가"), *Candidate->GetName());
			}
		}
	}

	if (BattleConfig.EnemySide.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 주변에 적이 없음"));
		bHasTriggered = false;
		return;
	}

	// ── 2. 플레이어 파티 전투 액터 비동기 스폰 ──
	const TArray<FName>& PartyIds = PartySys->GetPartyIds();
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 파티 멤버가 없음"));
		bHasTriggered = false;
		return;
	}

	// 전투 전환에 필요한 정보 캡처
	TWeakObjectPtr<AEncounterTriggerActor> WeakThis(this);
	TWeakObjectPtr<APlayerController> WeakPC;
	if (const APawn* Pawn = Cast<APawn>(OverlapActor))
		WeakPC = Cast<APlayerController>(Pawn->GetController());

	FName LeaderCharID = PartyIds[0];

	SpawnSub->AsyncSpawnCombatActors(PartyIds, GetActorTransform(),
		[WeakThis, BattleConfig, WeakPC, LeaderCharID](TArray<ACombatCharacterActor*> SpawnedActors) mutable
		{
			AEncounterTriggerActor* Self = WeakThis.Get();
			if (!Self) return;

			for (ACombatCharacterActor* Actor : SpawnedActors)
			{
				BattleConfig.PlayerSide.Add(Actor);
				UE_LOG(LogTemp, Log, TEXT("EncounterTrigger : 플레이어 전투 캐릭터 %s 스폰 완료"), *Actor->GetName());
			}

			if (BattleConfig.PlayerSide.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 플레이어 전투 캐릭터 스폰 실패"));
				Self->bHasTriggered = false;
				return;
			}

			// 전투 모드 진입: 필드 폰 숨김 → CombatCharacterActor 빙의
			APlayerController* PC = WeakPC.Get();
			if (PC)
			{
				if (UPartyActorSpawnSubsystem* SpawnSubInner = Self->GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>())
				{
					SpawnSubInner->EnterCombatMode(PC, LeaderCharID);
				}
			}

			Self->ReadyforBattleSession(BattleConfig);
		});
}



void AEncounterTriggerActor::ReadyforBattleSession(const FBattleSessionConfig& Config)
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();
	
	if (!BattleSession)
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : BattleSessionSubsystem이 존재하지 않음"));
		bHasTriggered = false;
		return;
	}
	
	FGuid SessionID;
	const bool bSuccess = BattleSession->StartBattle(Config, SessionID);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log,TEXT("EncounterTrigger : BattleSession 시작. SessionID = %s"), *SessionID.ToString());
		TriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CreateCombatZone();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : BattleSession 시작 실패"));
		bHasTriggered = false;
	}
}

void AEncounterTriggerActor::CreateCombatZone()
{
	if (CombatZoneClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedZone = GetWorld()->SpawnActor<ACombatZoneActor>(
			CombatZoneClass, 
			GetActorLocation(), 
			GetActorRotation(), 
			SpawnParams
		);

		if (SpawnedZone)
		{
			UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger :  CombatZone 생성 성공"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger :  CombatZone 생성 실패 (CombatZoneClass 설정 되었는지 확인할 것.)"));
	}
}

void AEncounterTriggerActor::OnPlayerApproach()
{
	if (UPartyActorSpawnSubsystem*PartySpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>())
	{
		TArray<FName> PartyIds;
		PartySpawnSub->PreloadAssets(PartyIds);
	}
}




