#include "Combat/Camera/CameraSubsystem.h"
#include "Combat/Camera/CameraRigActor.h"
#include "Combat/Camera/CameraTargetInterface.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "EngineUtils.h"
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


void UCameraSubsystem::SaveFieldSnapshot()
{
    if (!CameraRig.IsValid()) return;

    FieldSnapshot.FLocation  = CameraRig->GetActorLocation();
    FieldSnapshot.FRotator  = CameraRig->GetActorRotation();
    FieldSnapshot.ArmLength = CameraRig->SpringArm
                              ? CameraRig->SpringArm->TargetArmLength
                              : 400.f;
    FieldSnapshot.Target    = CameraRig->GetCurrentTarget(); // 필드 타겟(JRPGPlayerPawn) 보관

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

    // CameraRig 위치/회전 즉시 복원 (박용석 : 회전은 ControlRotation 동기화쪽에서 전환)
    CameraRig->SetActorLocation(FieldSnapshot.FLocation);
    if (CameraRig->SpringArm)
        CameraRig->SpringArm->TargetArmLength = FieldSnapshot.ArmLength;

    SetTarget(FieldSnapshot.Target.Get());

    UE_LOG(LogTemp, Log, TEXT("UCameraSubsystem: 필드 카메라 스냅샷 복원 완료"));
}

void UCameraSubsystem::OnBattleEnded(const FBattleSessionSnapshot& /*Snapshot*/, EBattleEndReason /*Reason*/)
{
    RestoreFieldSnapshot();
}

void UCameraSubsystem::OnCharacterPossessed(AActor* NewCharacter)
{
    SetTarget(NewCharacter);
}