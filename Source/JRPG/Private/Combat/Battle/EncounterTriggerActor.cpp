#include "Combat/Battle/EncounterTriggerActor.h"
#include "Combat/Battle/CombatZoneSettingDataAsset.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CombatTransitionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "Game/JRPGPlayerPawn.h"
#include "Game/Companion/FieldCompanionSubsystem.h"
#include "Game/Companion/JRPGCompanionPawn.h"

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
	UFieldCompanionSubsystem* CompanionSub = World->GetSubsystem<UFieldCompanionSubsystem>();

	if (!PartySys || !SpawnSub)
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : Party 또는 PartyActorSpawn 서브시스템이 없음"));
		bHasTriggered = false;
		return;
	}

	// 적 감지 (월드에 이미 배치된 CombatCharacterActor)
	FBattleSessionConfig BattleConfig;
	TSet<AActor*> AddedActors;
	AddedActors.Add(const_cast<AActor*>(OverlapActor));   

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

		// ICombatParticipantInterface을 부모로 가지고 있는 액터 중 Enemy이면서 살아있는 캐릭터만 추가
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

	// 플레이어 파티 전투 액터 비동기 스폰
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

	// 각 필드에 있는 폰의 위치와 회전 수집할 Map
	TMap<FName, FTransform> FieldTransforms;

	// JRPGPlayerPawn의 위치/회전
	if (const AJRPGPlayerPawn* PlayerPawn = Cast<AJRPGPlayerPawn>(OverlapActor))
	{
		FieldTransforms.Add(LeaderCharID, PlayerPawn->GetActorTransform());
	}

	// 나머지 파티원들도 
	TArray<AJRPGCompanionPawn*> Companions;
	if (CompanionSub)
		Companions = CompanionSub->GetSpawnedCompanions();

	for (AJRPGCompanionPawn* Companion : Companions)
	{
		if (!IsValid(Companion)) continue;
		FieldTransforms.Add(Companion->CurrentCharacterId, Companion->GetActorTransform());
	}

	// FEncounterContext 생성 (플레이어 위치 기준)
	FEncounterContext EncounterCtx;
	EncounterCtx.Trigger = EEncounterTrigger::Sight;
	EncounterCtx.TriggerActor = const_cast<AActor*>(OverlapActor);
	EncounterCtx.ZoneCenter = OverlapActor->GetActorLocation();
	EncounterCtx.ZoneSetting = ZoneSetting;
	EncounterCtx.TimestampReal = FPlatformTime::Seconds();
	EncounterCtx.EncounterToken = FGuid::NewGuid();

	SpawnSub->AsyncSpawnCombatActorsAtFieldPositions(PartyIds, FieldTransforms,
		[WeakThis, BattleConfig, WeakPC, LeaderCharID, EncounterCtx](TArray<ACombatCharacterActor*> SpawnedActors) mutable 
		{
			//스폰은 PartyActorSpawnSubsystem이 하고, 결과만 람다로 받아서 EncounterTriggerActor가 처리
			//PartyActorSpawnSubsystem에서 OnComplete(SpawnedActors)가 호출되어야(실제 스폰이 완료되어야) 아래 등록이 실행됨. 
			// 즉, SpawnedActors가 만들어지는 곳이 PartyActorSpawnSubsystem의 DoSpawn() 안쪽임 (람다 구현하다가 나도 헷갈려서 적어둠...)
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

			// 배틀세션 진입 후 후처리 -> 필드 폰 숨기고 CombatCharacterActor로 빙의
			APlayerController* PC = WeakPC.Get();
			if (PC)
			{
				if (UCombatTransitionSubsystem* TransitionSub = Self->GetWorld()->GetSubsystem<UCombatTransitionSubsystem>())
				{
					TransitionSub->EnterCombatMode(PC, LeaderCharID);
				}
			}

			// 배틀 세션 시작 (Zone 생성은 BattleSession이 담당)
			Self->ReadyforBattleSession(BattleConfig, EncounterCtx);
		});
}



void AEncounterTriggerActor::ReadyforBattleSession(const FBattleSessionConfig& Config, const FEncounterContext& InEncounterCtx) 
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();

	if (!BattleSession)
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : BattleSessionSubsystem이 존재하지 않음"));
		bHasTriggered = false;
		return;
	}

	FGuid SessionID;
	const bool bSuccess = BattleSession->StartBattle(Config, InEncounterCtx, SessionID);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("EncounterTrigger : BattleSession 시작. SessionID = %s"), *SessionID.ToString());
		TriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : BattleSession 시작 실패"));
		bHasTriggered = false;

		// 배틀 시작 실패 시 이미 생성된 CombatZone 정리
		if (SpawnedZone)
		{
			//SpawnedZone->Destroy();
			SpawnedZone = nullptr;
		}
	}
}


void AEncounterTriggerActor::OnPlayerApproach()
{
	if (UPartyActorSpawnSubsystem* PartySpawnSub = GetWorld()->GetSubsystem<UPartyActorSpawnSubsystem>())
	{
		TArray<FName> PartyIds;
		PartySpawnSub->PreloadAssets(PartyIds);
	}
}