
#include "Combat/Battle/EnemyEncounterComponent.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/CombatTransitionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Session/CombatZoneActor.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "Game/JRPGPlayerPawn.h"
#include "Game/Companion/FieldCompanionSubsystem.h"
#include "Game/Companion/JRPGCompanionPawn.h"
#include "Components/CapsuleComponent.h"

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
	UFieldCompanionSubsystem* CompanionSub = World->GetSubsystem<UFieldCompanionSubsystem>();
	
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
	
	const TArray<FName>& PartyIds = PartySys->GetPartyIds();
	if (PartyIds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterComponent : 파티 멤버가 없음"));
		bHasTriggered = false;
		return;
	}

	TWeakObjectPtr<UEnemyEncounterComponent> WeakThis(this);
	TWeakObjectPtr<APlayerController> WeakPC;
	if (const APawn* Pawn = Cast<APawn>(PlayerActor))
	{
		WeakPC = Pawn->GetController<APlayerController>();
	}

	FName LeaderCharID = PartyIds[0];

	TMap<FName, FTransform> FieldTransforms;

	if (const AJRPGPlayerPawn* PlayerPawn = Cast<AJRPGPlayerPawn>(PlayerActor))
	{
		FieldTransforms.Add(LeaderCharID, GetOwner()->GetActorTransform());
	}

	// 나머지 파티원들도 
	TArray<AJRPGCompanionPawn*> Companions;
	if (CompanionSub)
		Companions = CompanionSub->GetSpawnedCompanions();

	for (AJRPGCompanionPawn* Companion : Companions)
	{
		if (!IsValid(Companion)) 
			continue;

		FieldTransforms.Add(Companion->CurrentCharacterId, Companion->GetActorTransform());
	}

	SpawnSub->AsyncSpawnCombatActorsAtFieldPositions(PartyIds, FieldTransforms,
		[WeakThis, BattleConfig, WeakPC, LeaderCharID](TArray<ACombatCharacterActor*> SpawnedActors) mutable
		{
			//스폰은 PartyActorSpawnSubsystem이 하고, 결과만 람다로 받아서 EncounterTriggerActor가 처리
			//PartyActorSpawnSubsystem에서 OnComplete(SpawnedActors)가 호출되어야(실제 스폰이 완료되어야) 아래 등록이 실행됨. 
			// 즉, SpawnedActors가 만들어지는 곳이 PartyActorSpawnSubsystem의 DoSpawn() 안쪽임 (람다 구현하다가 나도 헷갈려서 적어둠...)
			UEnemyEncounterComponent* Self = WeakThis.Get();
			if (!Self) return;

			for (ACombatCharacterActor* Actor : SpawnedActors)
			{
				BattleConfig.PlayerSide.Add(Actor);
				UE_LOG(LogTemp, Log, TEXT("EncounterComponent : 플레이어 전투 캐릭터 %s 스폰 완료"), *Actor->GetName());
			}

			if (BattleConfig.PlayerSide.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("EncounterComponent : 플레이어 전투 캐릭터 스폰 실패"));
				Self->bHasTriggered = false;
				return;
			}

			// 배틀세션 진입 후 후처리 -> 필드 폰 숨기고 CombatCharacterActor로 빙의
			APlayerController* PC = WeakPC.Get();
			if (PC)
			{
				if (UCombatTransitionSubsystem* TransitionSub = Self->GetWorld()->GetSubsystem<UCombatTransitionSubsystem>())
				{
					TransitionSub->EnterCombatMode(PC, LeaderCharID);
				}
			}

			Self->ReadyForBattleSession(BattleConfig);
		});
}





void UEnemyEncounterComponent::ReadyForBattleSession(const FBattleSessionConfig& Config)
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();

	if (!BattleSession)
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterComponent : BattleSessionSubsystem이 존재하지 않음."));
		bHasTriggered = false;
		return;
	}

	FGuid SessionID;
	const bool bSucessed = BattleSession->StartBattle(Config, SessionID);

	if (bSucessed)
	{
		UE_LOG(LogTemp, Log, TEXT("EncounterComponent : BattleSession 시작. SessionID = %s"), *SessionID.ToString());
		TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CreateCombatZone();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("EncounterComponent : BattleSession 시작 실패, 기능 점검 다시 "));
		bHasTriggered = false;
	}
}

void UEnemyEncounterComponent::CreateCombatZone()
{
	if (IsValid(CombatZoneClass))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedZone = GetWorld()->SpawnActor<ACombatZoneActor>(
			CombatZoneClass,
			this->GetOwner()->GetActorLocation(),
			this->GetOwner()->GetActorRotation(),
			Params
		);

		if (IsValid(SpawnedZone))
		{
			UE_LOG(LogTemp, Warning, TEXT("EncounterComponent : CombatZoneActor 생성 완료"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("EncounterComponent :  생성 실패"))
		}


	}
}
