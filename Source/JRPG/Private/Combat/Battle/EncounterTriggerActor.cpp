#include "Combat/Battle/EncounterTriggerActor.h"
#include "Combat/Battle/CombatZoneSettingDataAsset.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CombatTransitionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Session/CombatZoneActor.h"
#include "Combat/Stats/HPComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "Game/JRPGPlayerPawn.h"
#include "Game/Companion/CompanionPawnController.h"
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
	TriggerVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);

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

FTransform AEncounterTriggerActor::CompanionFallbackTransform(const AActor* LeaderActor, const AJRPGCompanionPawn* Companion, int32 CompanionOrder) const
{
	if (!IsValid(LeaderActor))
	{
		return Companion ? Companion->GetActorTransform() : FTransform::Identity;
	}

	int32 PartyIndex = CompanionOrder + 1;
	if (const ACompanionPawnController* CompanionController = Companion ? Cast<ACompanionPawnController>(Companion->GetController()) : nullptr)
	{
		PartyIndex = FMath::Max(1, CompanionController->GetPartyIndex());
	}

	const FVector LeaderLocation = LeaderActor->GetActorLocation();
	const FVector LeaderForward = LeaderActor->GetActorForwardVector();
	const FRotator LeaderRotation = LeaderActor->GetActorRotation();

	float AngleOffset = (PartyIndex % 2 != 0) ? -45.0f : 45.0f;
	AngleOffset *= FMath::CeilToFloat(PartyIndex / 2.0f);

	const FVector Direction = LeaderForward.RotateAngleAxis(AngleOffset, FVector::UpVector);
	const FVector SpawnLocation = LeaderLocation - (Direction * 200.0f);
	return FTransform(LeaderRotation, SpawnLocation);
}



void AEncounterTriggerActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasTriggered || !OtherActor)
		return;

	// 전투 중이거나 전환 중이면 무시
	if (UBattleSessionSubsystem* BattleSub = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		if (BattleSub->IsBattleActive()) return;
	}
	if (UCombatTransitionSubsystem* TransSub = GetWorld()->GetSubsystem<UCombatTransitionSubsystem>())
	{
		if (TransSub->IsTransitioning()) return;
	}

	AJRPGPlayerPawn* PlayerPawn = Cast<AJRPGPlayerPawn>(OtherActor);
	if (!PlayerPawn)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Triggered by %s on trigger=%s at %s"),
		*GetNameSafe(OtherActor),
		*GetNameSafe(this),
		*GetActorLocation().ToString());

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

	const float SearchRadiusSq = FMath::Square(SearchRadius);
	int32 RawCandidateCount = 0;

	UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Search center=%s radius=%.1f"),
		*TriggerLocation.ToString(),
		SearchRadius);

	for (TActorIterator<ACombatCharacterActor> It(World); It; ++It)
	{
		ACombatCharacterActor* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}

		const float Dist2DSq = FVector::DistSquared2D(TriggerLocation, Candidate->GetActorLocation());
		if (Dist2DSq > SearchRadiusSq)
		{
			continue;
		}

		++RawCandidateCount;

		if (!IsValid(Candidate) || AddedActors.Contains(Candidate))
		{
			if (IsValid(Candidate))
			{
				UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Skip candidate=%s reason=%s"),
					*GetNameSafe(Candidate),
					AddedActors.Contains(Candidate) ? TEXT("AlreadyAdded") : TEXT("Invalid"));
			}
			continue;
		}

		const float Dist2D = FMath::Sqrt(Dist2DSq);

		// ICombatParticipantInterface을 부모로 가지고 있는 액터 중 Enemy이면서 살아있는 캐릭터만 추가
		if (ICombatParticipantInterface* Participant = Cast<ICombatParticipantInterface>(Candidate))
		{
			const bool bIsEnemyTeam = Participant->GetCombatTeam() == ECombatTeam::Enemy;
			const bool bAlive = Participant->GetHP() && !Participant->GetHP()->IsDead();
			UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Candidate=%s dist2D=%.1f team=%d alive=%s"),
				*GetNameSafe(Candidate),
				Dist2D,
				static_cast<int32>(Participant->GetCombatTeam()),
				bAlive ? TEXT("true") : TEXT("false"));

			if (bIsEnemyTeam && bAlive)
			{
				BattleConfig.EnemySide.Add(Candidate);
				AddedActors.Add(Candidate);
				UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Added enemy=%s"), *GetNameSafe(Candidate));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Candidate=%s dist2D=%.1f does not implement ICombatParticipantInterface"),
				*GetNameSafe(Candidate),
				Dist2D);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Raw candidates in radius=%d"), RawCandidateCount);

	UE_LOG(LogTemp, Warning, TEXT("[EncounterDebug][EncounterTriggerActor] Final enemy count=%d"), BattleConfig.EnemySide.Num());

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

	int32 CompanionOrder = 0;
	for (AJRPGCompanionPawn* Companion : Companions)
	{
		if (!IsValid(Companion)) continue;

		FTransform CompanionTransform = Companion->GetActorTransform();
		const float DistanceToLeaderSq = FVector::DistSquared2D(CompanionTransform.GetLocation(), OverlapActor->GetActorLocation());
		if (DistanceToLeaderSq > FMath::Square(MaxEncounterCompanionDistance))
		{
			CompanionTransform = CompanionFallbackTransform(OverlapActor, Companion, CompanionOrder);
			UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 멀어진 컴패니언 %s 전투 스폰 위치를 리더 근처로 보정함."), *Companion->GetName());
		}

		FieldTransforms.Add(Companion->CurrentCharacterId, CompanionTransform);
		++CompanionOrder;
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
			// Session과 Transition까지 성공해야 Spawn subsystem이 Actor 소유권을 확정한다.
			AEncounterTriggerActor* Self = WeakThis.Get();
			if (!Self) return false;

			for (ACombatCharacterActor* Actor : SpawnedActors)
			{
				BattleConfig.PlayerSide.Add(Actor);
				UE_LOG(LogTemp, Log, TEXT("EncounterTrigger : 플레이어 전투 캐릭터 %s 스폰 완료"), *Actor->GetName());
			}

			if (BattleConfig.PlayerSide.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger : 플레이어 전투 캐릭터 스폰 실패"));
				Self->bHasTriggered = false;
				return false;
			}

			if (!Self->ReadyforBattleSession(BattleConfig, EncounterCtx))
			{
				return false;
			}

			// 배틀세션 시작 성공 후 후처리 -> 필드 폰 숨기고 CombatCharacterActor로 빙의
			bool bEnteredCombat = false;
			APlayerController* PC = WeakPC.Get();
			if (PC)
			{
				if (UCombatTransitionSubsystem* TransitionSub = Self->GetWorld()->GetSubsystem<UCombatTransitionSubsystem>())
				{
					bEnteredCombat = TransitionSub->EnterCombatMode(PC, LeaderCharID);
				}
			}

			if (!bEnteredCombat)
			{
				UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : 전투 전환 실패. BattleSession을 중단합니다."));
				if (UBattleSessionSubsystem* BattleSession = Self->GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
				{
					BattleSession->AbortBattle("Encounter.EnterCombatFailed");
				}
				Self->bHasTriggered = false;
				return false;
			}

			return true;
		});
}



bool AEncounterTriggerActor::ReadyforBattleSession(const FBattleSessionConfig& Config, const FEncounterContext& InEncounterCtx) 
{
	UBattleSessionSubsystem* BattleSession = GetWorld()->GetSubsystem<UBattleSessionSubsystem>();

	if (!BattleSession)
	{
		UE_LOG(LogTemp, Error, TEXT("EncounterTrigger : BattleSessionSubsystem이 존재하지 않음"));
		bHasTriggered = false;
		return false;
	}

	FGuid SessionID;
	const bool bSuccess = BattleSession->StartBattle(Config, InEncounterCtx, SessionID);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("EncounterTrigger : BattleSession 시작. SessionID = %s"), *SessionID.ToString());
		TriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return true;
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
		return false;
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
