// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Battle/EncounterTriggerActor.h"

#include "EngineUtils.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Session/CombatZoneActor.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "Game/JRPGPlayerController.h"
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
	const AJRPGPlayerPawn* PlayerPawn  = Cast<AJRPGPlayerPawn>(OverlapActor);
	if (!IsValid(PlayerPawn))
		return;

	FBattleSessionConfig BattleConfig;
	TSet<AActor*> AddedActors;
	
	
	UGameInstance* GI = GetGameInstance();
	
	UPartyActorSpawnSubsystem *PartySpawnSub = GI ? GI->GetSubsystem<UPartyActorSpawnSubsystem>() : nullptr;
	UCombatCharacterRegistrySubsystem* Registry = GI ? GI->GetSubsystem<UCombatCharacterRegistrySubsystem>() : nullptr;
	UPartySubsystem* PartySys = GI ? GI->GetSubsystem<UPartySubsystem>() : nullptr;
	
	
	if (!Registry || !PartySys || !PartySpawnSub)
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : Registry 또는 Party, PartyActorSpawn 서브시스템이 없음"));
		bHasTriggered = false;
		return;
	}
	
	if (!PlayerPawn->CurrentCharacterId.IsNone())
	{
		ACombatCharacterActor* CurrentCombatChar = Cast<ACombatCharacterActor>(Registry->FindById(PlayerPawn->CurrentCharacterId));
		if (CurrentCombatChar)
		{
			if (ICombatParticipantInterface *Particpant = Cast<ICombatParticipantInterface>(CurrentCombatChar))
			{
				if (Particpant->GetCombatTeam() == ECombatTeam::Player && Particpant->GetHP() && !Particpant->GetHP()->IsDead())
				{
					BattleConfig.PlayerSide.Add(CurrentCombatChar);
					AddedActors.Add(CurrentCombatChar);
					UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 플레이어 캐릭터 %s 추가"), *PlayerPawn->CurrentCharacterId.ToString());
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : PlayerPawn의 CurrentCharacterId에 해당하는 CombatCharacterActor를 Registry에 없음"), *PlayerPawn->CurrentCharacterId.ToString());
		}	
	}
	
	TArray<AActor*> PartyMembers;
	PartySys->GetPartyMembers(PartyMembers);
	
	for (AActor* Member : PartyMembers)
	{
		if (!IsValid(Member) || AddedActors.Contains(Member))
			continue;	
		
		if (ICombatParticipantInterface* Participant = Cast<ICombatParticipantInterface>(Member))
		{
			if (Participant->GetCombatTeam() == ECombatTeam::Player && Participant->GetHP() && !Participant->GetHP()->IsDead())
			{
				BattleConfig.PlayerSide.Add(Member);
				AddedActors.Add(Member);
				UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 파티 멤버캐릭터  %s 추가"), *Member->GetName());
			}
		}
	}
	
	if (BattleConfig.PlayerSide.Num()==0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 래지스트리에 등록가능한 플레이어 파티원이 없음"));
		bHasTriggered = false;
		return;
	}
	
	
	
	// 적감지
	const FVector TriggerLocation = GetActorLocation();
	const float SearchRadius = 1000.f; 
	
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);
	
	TArray<FOverlapResult> OverlapResults;
	GetWorld()->OverlapMultiByObjectType(OverlapResults, TriggerLocation, FQuat::Identity, QueryParams, Sphere);
	
	//콜리전 감지 중복방지
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
				UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 적 캐릭터 %s 추가"), *Candidate->GetName());
			}
		}
	}
	
	if (BattleConfig.PlayerSide.Num()==0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger: 주변에 적이 없음"));
		bHasTriggered = false;
		return;
	}
	
	
	ReadyforBattleSession(BattleConfig);
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




