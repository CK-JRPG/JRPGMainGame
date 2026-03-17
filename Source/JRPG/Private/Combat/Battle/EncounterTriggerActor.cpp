// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Battle/EncounterTriggerActor.h"

#include "EngineUtils.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
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
	
	bHasTriggered = true;
	SearchCombatCharactersInRadius(OtherActor);
}

void AEncounterTriggerActor::SearchCombatCharactersInRadius(const AActor* OverlapActor)
{
	FBattleSessionConfig BattleConfig;
	
	if (!IsValid(OverlapActor) || !OverlapActor->IsA<AJRPGPlayerPawn>())
		return;
	
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPartySubsystem* Registry = GameInstance->GetSubsystem<UPartySubsystem>())
		{
			TArray<AActor*> ActivePartyMembers;
			Registry->GetPartyMembers(ActivePartyMembers);
			
			for (AActor* CombatActor : ActivePartyMembers)
			{
				if (const ICombatParticipantInterface* Participant = Cast<ICombatParticipantInterface>(CombatActor))
				{
					if (Participant->GetCombatTeam() == ECombatTeam::Player)
					{
						BattleConfig.PlayerSide.Add(CombatActor);
						UE_LOG(LogTemp, Warning, TEXT("Encounter : 플레이어 측 캐릭터 추가 완료. Actor: %s"), *CombatActor->GetName());
					}
				}
			}
		}
	}
	
	if (BattleConfig.PlayerSide.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Encounter : 레지스트리에 등록된 플레이어가 없음"));
		//bHasTriggered = false;
		//return;
	}
	
	
	
	const FVector TriggerLocation = GetActorLocation();
	float SearchRadius = 1000.0f;
	
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn); 
	
	TArray<FOverlapResult> ActiveEnemiesResults;
	
	bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
		ActiveEnemiesResults, 
		TriggerLocation, 
		FQuat::Identity, 
		ObjectQueryParams, 
		Sphere
	);
	if (bHasOverlap)
	{
		// TODO : 적 캐릭터 감지 개선 : 각 캐릭터당 한번만 감지되도록 바꿔야함.
		//반경 내에 스캔된 적 COnfig에 추가하기
		for (const FOverlapResult& Results : ActiveEnemiesResults)
		{
			if (AActor* ActiveEnemy = Results.GetActor())
			{
				if (const ICombatParticipantInterface* Participant = Cast<ICombatParticipantInterface>(ActiveEnemy))
				{
					// 팀이 Enemy이고 HP가 유효한지 검증
					if (Participant->GetCombatTeam() == ECombatTeam::Enemy && Participant->GetHP() != nullptr)
					{
						BattleConfig.EnemySide.Add(ActiveEnemy);
						UE_LOG(LogTemp, Warning, TEXT("Encounter : 적 측 캐릭터 추가 완료. Actor: %d"), BattleConfig.EnemySide.Num());

					}
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




