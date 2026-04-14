#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Camera/CameraRigActor.h"
#include "Combat/Camera/CameraTargetInterface.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

void UCameraSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    for (TActorIterator<ACameraRigActor> It(&InWorld); It; ++It)
    {
        CameraRig = *It;
        break;
    }

    if (!CameraRig.IsValid())
    {
        UE_LOG(LogTemp, Error,
            TEXT("UCameraSubsystem: 월드에 ACameraRigActor가 없음. - 자동 스폰"));
        
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        CameraRig = InWorld.SpawnActor<ACameraRigActor>(
            ACameraRigActor::StaticClass(),
            FTransform::Identity,
            Params
        );
    }

    if (UBattleSessionSubsystem* BS = InWorld.GetSubsystem<UBattleSessionSubsystem>())
    {
        BS->OnBattleEnded.AddUObject(this, &UCameraSubsystem::OnBattleEnded);
    }
}

void UCameraSubsystem::SetTarget(AActor* NewTarget)
{
    if (!CameraRig.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UCameraSubsystem::SetTarget - CameraRig 없음"));
        return;
    }

    // ICameraTargetInterface를 구현했는지 검사
    if (NewTarget && !Cast<ICameraTargetInterface>(NewTarget))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCameraSubsystem::SetTarget - %s 가 ICameraTargetInterface를 구현하지 않음"),
            *GetNameSafe(NewTarget));
        return;
    }

    CameraRig->SetCameraTarget(NewTarget);
    
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        FViewTargetTransitionParams Params;
        Params.BlendTime = 0.0f;
        PC->SetViewTarget(CameraRig.Get(), Params);
        
        UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem : ViewTarget -> CameraRig, 추적 대상 -> %s"), *GetNameSafe(NewTarget));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UCameraSubsystem::SetTarget - PlayerController 없음"));
    }
}


void UCameraSubsystem::SetTargetSmooth(AActor* NewTarget)
{
    if (!CameraRig.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UCameraSubsystem::SetTargetSmooth - CameraRig 없음"));
        return;
    }

    if (NewTarget && !Cast<ICameraTargetInterface>(NewTarget))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCameraSubsystem::SetTargetSmooth - %s 가 ICameraTargetInterface를 구현하지 않음"),
            *GetNameSafe(NewTarget));
        return;
    }

    // 위치/회전/ArmLength 스냅 없이 타겟만 변경 (Tick에서 보간 처리)
    CameraRig->SetCameraTargetSmooth(NewTarget);
    
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        FViewTargetTransitionParams Params;
        Params.BlendTime = 0.0f;
        PC->SetViewTarget(CameraRig.Get(), Params);
        
        UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem : ViewTarget -> CameraRig (스무스 전환), 추적 대상 -> %s"), *GetNameSafe(NewTarget));
    }
}


void UCameraSubsystem::SaveFieldSnapshot(AActor* OverrideTarget)
{
    if (!CameraRig.IsValid()) return;

    FieldSnapshot.FLocation  = CameraRig->GetActorLocation();
    FieldSnapshot.FRotator  = CameraRig->GetActorRotation();
    FieldSnapshot.ArmLength = CameraRig->SpringArm
                              ? CameraRig->SpringArm->TargetArmLength
                              : 550.f;
    FieldSnapshot.Target = OverrideTarget ? OverrideTarget : CameraRig->GetCurrentTarget();
    FieldSnapshot.bHasControlRotation = false;

    if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        FieldSnapshot.ControlRotation = PC->GetControlRotation();
        FieldSnapshot.bHasControlRotation = true;
    }

    UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem: 필드 카메라 스냅샷 저장 완료"));
}

void UCameraSubsystem::RestoreFieldSnapshot()
{
    if (!CameraRig.IsValid() || !FieldSnapshot.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCameraSubsystem::RestoreFieldSnapshot — CameraRig 없거나 스냅샷 없음"));
        return;
    }

    if (FieldSnapshot.bHasControlRotation)
    {
        if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
        {
            PC->SetControlRotation(FieldSnapshot.ControlRotation);
        }
    }

    // (가능한 경우) ControlRotation 동기화 후 CameraRig 위치/회전/거리 복원
    CameraRig->SetActorLocation(FieldSnapshot.FLocation);
    CameraRig->SetActorRotation(FieldSnapshot.FRotator);
    if (CameraRig->SpringArm)
    {
        CameraRig->SetArmLength(FieldSnapshot.ArmLength, true);
    }

    SetTargetSmooth(FieldSnapshot.Target.Get());

    UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem: 필드 카메라 스냅샷 복원 완료"));
}

bool UCameraSubsystem::GetSavedFieldControlRotation(FRotator& OutControlRotation) const
{
    if (!FieldSnapshot.bHasControlRotation)
    {
        return false;
    }

    OutControlRotation = FieldSnapshot.ControlRotation;
    return true;
}

void UCameraSubsystem::AdjustZoom(float NormalizedDelta)
{
    if (CameraRig.IsValid())
        CameraRig->AdjustZoom(NormalizedDelta * CameraRig->GetZoomStep());
}

void UCameraSubsystem::ResetZoom()
{
    if (CameraRig.IsValid())
        CameraRig->ResetZoom();
}

void UCameraSubsystem::LockOnEnemy()
{
    RefreshEnemyList();

    if (CachedEnemies.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCameraSubsystem::FocusEnemy : 락온할 적이 없음"));
        return;
    }

    // 플레이어 폰 기준으로 가장 가까운 적 선택
    AActor* PlayerPawn = nullptr;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PlayerPawn = PC->GetPawn();
    }

    int32 BestIndex = 0;
    if (PlayerPawn)
    {
        float BestDistSq = FLT_MAX;
        const FVector PlayerLoc = PlayerPawn->GetActorLocation();

        for (int32 i = 0; i < CachedEnemies.Num(); ++i)
        {
            if (AActor* Enemy = CachedEnemies[i].Get())
            {
                const float DistSq = FVector::DistSquared(PlayerLoc, Enemy->GetActorLocation());
                if (DistSq < BestDistSq)
                {
                    BestDistSq = DistSq;
                    BestIndex = i;
                }
            }
        }
    }

    LockedOnEnemyIndex = BestIndex;
    LockedOnEnemy = CachedEnemies[LockedOnEnemyIndex].Get();
    bLockedOn = true;

    if (CameraRig.IsValid())
    {
        CameraRig->SetLockOnTarget(LockedOnEnemy.Get());
    }

    UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem: 적 포커싱 시작 -> %s (인덱스 %d/%d)"),
        *GetNameSafe(LockedOnEnemy.Get()), LockedOnEnemyIndex + 1, CachedEnemies.Num());
}

void UCameraSubsystem::CycleLockOnEnemy(int32 Direction)
{
    if (!bLockedOn) 
        return;

    RefreshEnemyList();

    if (CachedEnemies.Num() == 0)
    {
        ClearLockOn();
        return;
    }

    // 현재 락온 중인 적이 죽었거나 유효하지 않으면 인덱스 조정
    if (!LockedOnEnemy.IsValid())
    {
        LockedOnEnemyIndex = FMath::Clamp(LockedOnEnemyIndex, 0, CachedEnemies.Num() - 1);
    }
    else
    {
        // 새 목록에서 현재 적의 인덱스를 다시 찾기
        const int32 NewIdx = CachedEnemies.IndexOfByPredicate([this](const TWeakObjectPtr<AActor>& Elem) 
            { 
                return Elem.Get() == LockedOnEnemy.Get(); 
            });

        if (NewIdx != INDEX_NONE)
        {
            LockedOnEnemyIndex = NewIdx;
        }
        else
        {
            LockedOnEnemyIndex = FMath::Clamp(LockedOnEnemyIndex, 0, CachedEnemies.Num() - 1);
        }
    }

    // 순환 인덱스: +Num으로 음수 Direction 처리
    const int32 EnemyCount = CachedEnemies.Num();
    if (EnemyCount == 0) { ClearLockOn(); return; }
    LockedOnEnemyIndex = (LockedOnEnemyIndex + Direction + EnemyCount) % EnemyCount;
    LockedOnEnemy = CachedEnemies[LockedOnEnemyIndex].Get();

    // 락온 타겟 전환
    if (CameraRig.IsValid())
    {
        CameraRig->SetLockOnTarget(LockedOnEnemy.Get());
    }

    UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem: 적 락온 순환 -> %s (인덱스 %d/%d)"),
        *GetNameSafe(LockedOnEnemy.Get()), LockedOnEnemyIndex + 1, CachedEnemies.Num());
}

void UCameraSubsystem::ClearLockOn()
{
    if (!bLockedOn) return;

    bLockedOn = false;
    LockedOnEnemyIndex = INDEX_NONE;
    LockedOnEnemy.Reset();
    CachedEnemies.Empty();

    // 락온 해제 (카메라 타겟은 플레이어에 이미 있으므로 전환 불필요)
    if (CameraRig.IsValid())
        CameraRig->ClearLockOnTarget();

    UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem: 적 락온 해제"));
}

void UCameraSubsystem::RefreshEnemyList()
{
    CachedEnemies.Empty();

    UBattleSessionSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
    if (!Battle || !Battle->IsBattleActive()) 
        return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
        return;

    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn)
        return;

    TArray<AActor*> Opponents;
    Battle->GetOpponentsFor(PlayerPawn, Opponents);

    // ICameraTargetInterface를 구현한 적만 필터링
    for (AActor* Enemy : Opponents)
    {
        if (Enemy && Enemy->Implements<UCameraTargetInterface>())
        {
            CachedEnemies.Add(Enemy);
        }
    }

    // 플레이어와의 거리 기준 정렬
    const FVector PlayerLoc = PlayerPawn->GetActorLocation();
    CachedEnemies.Sort([&PlayerLoc](const TWeakObjectPtr<AActor>& A, const TWeakObjectPtr<AActor>& B)
        {
            const AActor* ActorA = A.Get();
            const AActor* ActorB = B.Get();
            if (!ActorA || !ActorB) 
                return (ActorA != nullptr) && (ActorB == nullptr);

            return FVector::DistSquared(PlayerLoc, ActorA->GetActorLocation()) < FVector::DistSquared(PlayerLoc, ActorB->GetActorLocation());
        });
}

void UCameraSubsystem::OnBattleEnded(const FBattleSessionSnapshot& /*Snapshot*/, EBattleEndReason /*Reason*/)
{
    bLockedOn = false;
    LockedOnEnemyIndex = INDEX_NONE;
    LockedOnEnemy.Reset();
    CachedEnemies.Empty();

    if (CameraRig.IsValid())
    {
        CameraRig->ClearLockOnTarget();
    }
}

void UCameraSubsystem::OnCharacterPossessed(AActor* NewCharacter)
{
    if (bLockedOn)
        return;

    SetTargetSmooth(NewCharacter);
}