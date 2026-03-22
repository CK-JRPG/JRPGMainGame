#include "Game/Companion/CompanionPawnController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

ACompanionPawnController::ACompanionPawnController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	CurrentState = ECompanionAdventureState::None;
}

void ACompanionPawnController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(InPawn))
	{
		if (UCharacterMovementComponent* MoveComp = ControlledCharacter->GetCharacterMovement())
		{
			MoveComp->bUseRVOAvoidance = false; 
		}
	}
	
	if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
		CrowdComp->SetCrowdSeparationWeight(10.0f);
	}

	// ---------------------------------------------------------
	// 자기 자신의 Party Index 자동 부여 로직
	// ---------------------------------------------------------
	TArray<AActor*> FoundControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACompanionPawnController::StaticClass(), FoundControllers);
	
	SetPartyIndex(FoundControllers.Num());
	
	UE_LOG(LogTemp, Log, TEXT("Companion AI [%s] 자동 인덱스 부여됨: %d"), *GetName(), PartyIndex);

	SetAdventureState(ECompanionAdventureState::Idle);
}

void ACompanionPawnController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!IsValid(LeaderActor))
	{
		if (AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			SetLeaderActor(PlayerActor);
			UE_LOG(LogTemp, Log, TEXT("Companion AI: 드디어 리더를 찾았습니다! (%s)"), *PlayerActor->GetName());
		}
		else
		{
			// 아직 플레이어가 없다면 FSM 로직을 돌리지 않고 이번 프레임 넘김
			return;
		}
	}
	
	SyncMovementSpeedWithLeader();

	switch (CurrentState)
	{
	case ECompanionAdventureState::None:
		break;
	case ECompanionAdventureState::Idle:
		UpdateIdle(DeltaTime);
		break;
	case ECompanionAdventureState::FollowLeader:
		UpdateFollowLeader(DeltaTime);
		break;
	case ECompanionAdventureState::TeleportCatchUp:
	default:
		break;
	}
}

void ACompanionPawnController::SetLeaderActor(AActor* InLeader)
{
	LeaderActor = InLeader;
	if (IsValid(LeaderActor))
	{
		SetAdventureState(ECompanionAdventureState::FollowLeader);
	}
}

void ACompanionPawnController::SetPartyIndex(int32 InIndex)
{
	PartyIndex = InIndex;
}

void ACompanionPawnController::SetAdventureState(ECompanionAdventureState NewState)
{
	if (CurrentState == NewState) return;

	OnExitState(CurrentState);

	CurrentState = NewState;

	OnEnterState(CurrentState);
}

void ACompanionPawnController::OnEnterState(ECompanionAdventureState State)
{
	switch (State)
	{
	case ECompanionAdventureState::Idle:
		// 대기 모션 재생, 무기 집어넣기 등
		break;

	case ECompanionAdventureState::FollowLeader:

		break;

	case ECompanionAdventureState::TeleportCatchUp:
		// 1. 카메라에 보이지 않을 때만 텔레포트 시도
		if (!IsInCameraFrustum())
		{
			FVector FormationPos = GetFormationLocation();
			GetPawn()->SetActorLocation(FormationPos);
		}

		SetAdventureState(ECompanionAdventureState::Idle);
		break;

	default:
		//UE_LOG(LogTemp, Error, TEXT("ACompanionPawnController::OnEnterState: 처리되지 않은 AI 상태가 있습니다! (%d)"), (int32)CurrentState);

		break;
	}
}

void ACompanionPawnController::OnExitState(ECompanionAdventureState State)
{
	switch (State)
	{
	case ECompanionAdventureState::FollowLeader:
		StopMovement();
		break;

	default:
		//UE_LOG(LogTemp, Error, TEXT("ACompanionPawnController::OnExitState: 처리되지 않은 AI 상태가 있습니다! (%d)"), (int32)CurrentState);

		break;
	}
}

void ACompanionPawnController::UpdateIdle(float DeltaTime)
{
	if (!IsValid(LeaderActor)) return;

	float DistanceToLeader = GetPawn()->GetDistanceTo(LeaderActor);

	if (DistanceToLeader > TeleportRadius)
	{
		SetAdventureState(ECompanionAdventureState::TeleportCatchUp);
	}
	else if (DistanceToLeader > FollowStartRadius)
	{
		SetAdventureState(ECompanionAdventureState::FollowLeader);
	}
}

void ACompanionPawnController::UpdateFollowLeader(float DeltaTime)
{
	if (!IsValid(LeaderActor))
	{
		SetAdventureState(ECompanionAdventureState::Idle);
		return;
	}

	float DistanceToLeader = GetPawn()->GetDistanceTo(LeaderActor);

	if (DistanceToLeader > TeleportRadius)
	{
		SetAdventureState(ECompanionAdventureState::TeleportCatchUp);
		return;
	}

	if (DistanceToLeader <= FollowStopRadius)
	{
		SetAdventureState(ECompanionAdventureState::Idle);
		return;
	}

	// 지속적으로 대형 위치로 갱신 이동
	MoveToLocation(GetFormationLocation(), 50.0f, false, true, true, true, 0, false);
}

void ACompanionPawnController::SyncMovementSpeedWithLeader()
{
	if (!IsValid(LeaderActor) || !GetPawn()) return;

	ACharacter* LeaderChar = Cast<ACharacter>(LeaderActor);
	ACharacter* MyChar = Cast<ACharacter>(GetPawn());

	if (LeaderChar && MyChar)
	{
		// 리더의 걷기/뛰기 최고 속도를 동료 AI에게도 똑같이 적용
		MyChar->GetCharacterMovement()->MaxWalkSpeed = LeaderChar->GetCharacterMovement()->MaxWalkSpeed;
	}
}

FVector ACompanionPawnController::GetFormationLocation() const
{
	if (!IsValid(LeaderActor)) return FVector::ZeroVector;

	// 부채꼴 대형 계산 로직 (PartyIndex 기반)
	FVector LeaderLocation = LeaderActor->GetActorLocation();
	FVector LeaderForward = LeaderActor->GetActorForwardVector();
	
	float AngleOffset = (PartyIndex % 2 != 0) ? -45.0f : 45.0f;
	AngleOffset *= FMath::CeilToFloat(PartyIndex / 2.0f);
	
	FVector Direction = LeaderForward.RotateAngleAxis(AngleOffset, FVector::UpVector);
	
	FVector TargetPos = LeaderLocation - (Direction * FollowStopRadius);
	
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation ProjectedLocation;
	if (NavSys && NavSys->ProjectPointToNavigation(TargetPos, ProjectedLocation))
	{
		return ProjectedLocation.Location;
	}

	return TargetPos;
}

bool ACompanionPawnController::IsInCameraFrustum() const
{
	if (!GetPawn()) return false;
	
	float PawnLastRenderTime = GetPawn()->GetLastRenderTime();
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	return (CurrentTime - PawnLastRenderTime) < 0.2f;
}
