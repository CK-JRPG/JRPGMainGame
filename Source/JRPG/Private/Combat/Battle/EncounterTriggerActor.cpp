// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Battle/EncounterTriggerActor.h"

#include "EngineUtils.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Session/CombatZoneActor.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/BoxComponent.h"
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
	SearchCombatCharactersInRadius(PlayerPawn);
}

void AEncounterTriggerActor::SearchCombatCharactersInRadius(AJRPGPlayerPawn* TriggeringPlayer)
{
	FBattleSessionConfig BattleConfig;
	
	if (TriggeringPlayer)
	{
		BattleConfig.PlayerSide.Add(TriggeringPlayer);
		UE_LOG(LogTemp, Warning, TEXT("Encounter : 플레이어 직접 추가 완료."));
	}
	
	
	const FVector TriggerLocation = GetActorLocation();
	float SearchRadius = 1000.0f;
	
	float SearchRadiusSquared = FMath::Square(SearchRadius);
	
	//적 COnfig에 추가하기
	for (TActorIterator<ACombatCharacterActor> It(GetWorld()); It; ++It)
	{
		ACombatCharacterActor* CombatActor = *It;

		if (IsValid(CombatActor))
		{
			float DistSq = FVector::DistSquared(TriggerLocation, CombatActor->GetActorLocation());

			if (DistSq <= SearchRadiusSquared)
			{
				ECombatTeam Team = CombatActor->GetCombatTeam();
			
				if (Team == ECombatTeam::Enemy)
				{
					BattleConfig.EnemySide.Add(CombatActor);
					UE_LOG(LogTemp, Warning, TEXT("Encounter : 적 감지 및 FBattleSessionConfig에 추가 되었음."));

				}
			}
		}
	}
	if (BattleConfig.EnemySide.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Encounter : 주변에 적이 없음"));
		bHasTriggered = false;
		return;
	}
	
	ReadyforBattleSession(BattleConfig);
}

void AEncounterTriggerActor::ReadyforBattleSession(const FBattleSessionConfig& Config)
{
	if (UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		FGuid SessionID;
		bool bSuccess = BattleSession->StartBattle(Config, SessionID);
		
		if (bSuccess)
		{
			UE_LOG(LogTemp, Warning, TEXT("Encounter : 배틀 세션 시작 성공. SessionID: %s"), *SessionID.ToString());
			TriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			CreateCombatZone();
		}
		
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Encounter : 배틀 세션 시작 실패"));
			bHasTriggered = false; 
			
			CreateCombatZone(); // 나중에 배틀세션 기능 점검 끝나면 해당 부분 제거할 것.

		}
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




