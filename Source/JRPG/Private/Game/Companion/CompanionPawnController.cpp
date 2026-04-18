#include "Game/Companion/CompanionPawnController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ACompanionPawnController::ACompanionPawnController()
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
			MoveComp->bOrientRotationToMovement = true;
		}
	}

	// 자기 자신의 Party Index 자동 부여
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
			UE_LOG(LogTemp, Log, TEXT("Companion AI: 리더를 찾았습니다! (%s)"), *PlayerActor->GetName());
		}
		else
		{
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
		UpdateTeleportCatchUp(DeltaTime);
		break;
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
		break;

	case ECompanionAdventureState::FollowLeader:
		break;

	case ECompanionAdventureState::TeleportCatchUp:
		CatchUpTimer = 0.f;
		// 카메라 밖이면 즉시 텔레포트
		if (!IsInCameraFrustum())
		{
			if (APawn* MyPawn = GetPawn())
			{
				const FVector FormationPos = GetFormationLocation();
				MyPawn->TeleportTo(FormationPos, MyPawn->GetActorRotation());
				UE_LOG(LogTemp, Log, TEXT("Companion AI: 텔레포트 완료 (카메라 밖)"));
			}
			SetAdventureState(ECompanionAdventureState::Idle);
		}
		// 카메라 안이면 UpdateTeleportCatchUp에서 빠른 속도로 추적
		break;

	default:
		break;
	}
}

void ACompanionPawnController::OnExitState(ECompanionAdventureState State)
{
	switch (State)
	{
	case ECompanionAdventureState::FollowLeader:
	case ECompanionAdventureState::TeleportCatchUp:
		// 직접 이동이므로 별도 정지 불필요 (AddInputVector 중단이면 자동 감속)
		break;
	default:
		break;
	}
}

void ACompanionPawnController::UpdateIdle(float DeltaTime)
{
	if (!IsValid(LeaderActor) || !GetPawn()) return;

	const float DistanceToLeader = GetPawn()->GetDistanceTo(LeaderActor);

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
	if (!IsValid(LeaderActor) || !GetPawn())
	{
		SetAdventureState(ECompanionAdventureState::Idle);
		return;
	}

	const float DistanceToLeader = GetPawn()->GetDistanceTo(LeaderActor);

	if (DistanceToLeader > TeleportRadius)
	{
		SetAdventureState(ECompanionAdventureState::TeleportCatchUp);
		return;
	}

	const FVector FormationPos = GetFormationLocation();
	const float DistToFormation = FVector::Dist2D(GetPawn()->GetActorLocation(), FormationPos);

	// 대형 도착 판정 (FollowStopRadius의 절반)
	if (DistToFormation <= FollowStopRadius * 0.5f)
	{
		SetAdventureState(ECompanionAdventureState::Idle);
		return;
	}

	// NavMesh 미사용 — 직접 이동
	MoveDirectlyToward(FormationPos, DeltaTime);
}

void ACompanionPawnController::UpdateTeleportCatchUp(float DeltaTime)
{
	// 카메라 안에 보이는 상태에서 진입했을 때: 빠른 속도로 쫓아감
	if (!IsValid(LeaderActor) || !GetPawn())
	{
		SetAdventureState(ECompanionAdventureState::Idle);
		return;
	}

	CatchUpTimer += DeltaTime;

	// 쫓아가다가 카메라 밖이 되면 즉시 텔레포트
	if (!IsInCameraFrustum())
	{
		if (APawn* MyPawn = GetPawn())
		{
			const FVector FormationPos = GetFormationLocation();
			MyPawn->TeleportTo(FormationPos, MyPawn->GetActorRotation());
			UE_LOG(LogTemp, Log, TEXT("Companion AI: 추적 중 카메라 밖 → 텔레포트 완료"));
		}
		SetAdventureState(ECompanionAdventureState::Idle);
		return;
	}

	// 제한 시간 초과 시 강제 텔레포트
	if (CatchUpTimer >= TeleportCatchUpTimeoutSeconds)
	{
		if (APawn* MyPawn = GetPawn())
		{
			const FVector FormationPos = GetFormationLocation();
			MyPawn->TeleportTo(FormationPos, MyPawn->GetActorRotation());
			UE_LOG(LogTemp, Log, TEXT("Companion AI: 추적 제한 시간 초과 → 강제 텔레포트"));
		}
		SetAdventureState(ECompanionAdventureState::Idle);
		return;
	}

	// 빠른 속도로 대형 위치를 향해 이동
	const FVector FormationPos = GetFormationLocation();
	const float DistToFormation = FVector::Dist2D(GetPawn()->GetActorLocation(), FormationPos);

	if (DistToFormation <= FollowStopRadius)
	{
		SetAdventureState(ECompanionAdventureState::FollowLeader);
		return;
	}

	// CatchUpSpeedMultiplier 배속으로 추적
	if (ACharacter* MyChar = Cast<ACharacter>(GetPawn()))
	{
		const float OriginalSpeed = MyChar->GetCharacterMovement()->MaxWalkSpeed;
		MyChar->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeed * CatchUpSpeedMultiplier;
		MoveDirectlyToward(FormationPos, DeltaTime);
		MyChar->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeed;
	}
}

void ACompanionPawnController::MoveDirectlyToward(const FVector& Destination, float DeltaTime)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	ACharacter* MyChar = Cast<ACharacter>(MyPawn);
	if (!MyChar) return;

	UCharacterMovementComponent* MoveComp = MyChar->GetCharacterMovement();
	if (!MoveComp) return;

	FVector Dir = Destination - MyPawn->GetActorLocation();
	Dir.Z = 0.f;

	const float Dist = Dir.Size();
	if (Dist < 1.0f) return;

	Dir /= Dist; // Normalize

	// CharacterMovementComponent에 입력 전달
	MyChar->AddMovementInput(Dir, 1.0f);
}

void ACompanionPawnController::SyncMovementSpeedWithLeader()
{
	if (!IsValid(LeaderActor) || !GetPawn()) return;

	ACharacter* LeaderChar = Cast<ACharacter>(LeaderActor);
	ACharacter* MyChar = Cast<ACharacter>(GetPawn());

	if (LeaderChar && MyChar)
	{
		MyChar->GetCharacterMovement()->MaxWalkSpeed = LeaderChar->GetCharacterMovement()->MaxWalkSpeed;
	}
}

FVector ACompanionPawnController::GetFormationLocation() const
{
	if (!IsValid(LeaderActor)) return FVector::ZeroVector;

	const FVector LeaderLocation = LeaderActor->GetActorLocation();
	const FVector LeaderForward = LeaderActor->GetActorForwardVector();
	
	float AngleOffset = (PartyIndex % 2 != 0) ? -45.0f : 45.0f;
	AngleOffset *= FMath::CeilToFloat(PartyIndex / 2.0f);
	
	const FVector Direction = LeaderForward.RotateAngleAxis(AngleOffset, FVector::UpVector);
	
	// NavMesh 미사용 — 직접 계산한 위치 반환
	return LeaderLocation - (Direction * FollowStopRadius);
}

bool ACompanionPawnController::IsInCameraFrustum() const
{
	if (!GetPawn()) return false;
	
	const float PawnLastRenderTime = GetPawn()->GetLastRenderTime();
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	
	return (CurrentTime - PawnLastRenderTime) < 0.2f;
}
