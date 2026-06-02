#include "Combat/Characters/CombatCharacterActor.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Skills/SkillComponent.h"
#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"
#include "Combat/AI/CombatAIActionSelectorComponent.h"
#include "Combat/AI/CombatCharacterActorAIController.h"
#include "Combat/AI/CombatPartyAIComponent.h"
#include "Combat/Battle/EnemyEncounterComponent.h"
#include "Combat/Battle/DirectionalDamageComponent.h"
#include "Combat/AI/EnemyAIController.h"
#include "Combat/Items/CombatItemComponent.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Presentation/CombatVFXComponent.h"
#include "Combat/Presentation/TargetGuideLineComponent.h"
#include "Combat/Motion/CombatMotionComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Movement/JRPGCharacterMovementComponent.h"

#include "Combat/Stats/HPComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/Stats/CombatStatsComponent.h"
#include "Combat/Session/CombatZoneTrackerComponent.h"

#include "UI/Combat/EnemyHPBarWidget.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"



ACombatCharacterActor::ACombatCharacterActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJRPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = ACombatCharacterActorAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	CharacterComp = CreateDefaultSubobject<UCombatCharacterComponent>(TEXT("CombatCharacterComponent"));

	HPComp = CreateDefaultSubobject<UHPComponent>(TEXT("HPComponent"));
	APComp = CreateDefaultSubobject<UAPComponent>(TEXT("APComponent"));
	SPComp = CreateDefaultSubobject<USPComponent>(TEXT("SPComponent"));

	StatsComp = CreateDefaultSubobject<UCombatStatsComponent>(TEXT("CombatStatsComponent"));
	ActionComp = CreateDefaultSubobject<UCombatActionComponent>(TEXT("CombatActionComponent"));
	SkillComp = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	StatusComp = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));
	GroggyComp = CreateDefaultSubobject<UGroggyComponent>(TEXT("GroggyComponent"));
	ThreatComp = CreateDefaultSubobject<UThreatComponent>(TEXT("ThreatComponent"));
	AIActionSelectorComp = CreateDefaultSubobject<UCombatAIActionSelectorComponent>(TEXT("CombatAIActionSelectorComponent"));
	ItemComp = CreateDefaultSubobject<UCombatItemComponent>(TEXT("CombatItemComponent"));
	PresentationComp = CreateDefaultSubobject<UCombatPresentationComponent>(TEXT("CombatPresentationComponent"));
	VFXComp = CreateDefaultSubobject<UCombatVFXComponent>(TEXT("CombatVFXComponent"));
	TargetGuideLineComp = CreateDefaultSubobject<UTargetGuideLineComponent>(TEXT("TargetGuideLineComponent"));
	MotionComp = CreateDefaultSubobject<UCombatMotionComponent>(TEXT("CombatMotionComponent"));
	LocomotionComp = CreateDefaultSubobject<ULocomotionComponent>(TEXT("LocomotionComponent"));
	EnemyEncounterComp = CreateDefaultSubobject<UEnemyEncounterComponent>(TEXT("EnemyEncounterComponent"));
	ZoneTrackerComp = CreateDefaultSubobject<UCombatZoneTrackerComponent>(TEXT("CombatZoneTracker"));
	CombatPartyAIComp = CreateDefaultSubobject<UCombatPartyAIComponent>(TEXT("CombatPartyAIComponent"));
	DirectionalDamageComp = CreateDefaultSubobject<UDirectionalDamageComponent>(TEXT("DirectionalDamageComponent"));
	// PartyAIComp는 BeginPlay에서 팀 확인 후 활성화
	CombatPartyAIComp->PrimaryComponentTick.bStartWithTickEnabled = false;

	// HPBarWidget
	HPBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComponent"));
	HPBarWidgetComponent->SetupAttachment(RootComponent);

	HPBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	HPBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComponent->SetDrawSize(FVector2D(150.f, 20.f));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	}
}

void ACombatCharacterActor::BeginPlay()
{
	Super::BeginPlay();

	// 사망 이벤트 바인딩
	if (HPComp)
	{
		HPComp->OnDeath.AddUObject(this, &ACombatCharacterActor::HandleOnDeath);
	}
	
	// 팀에 따라 AI 컴포넌트 활성화/비활성화
	if (CharacterComp)
	{
		const ECombatTeam Team = CharacterComp->GetTeam();
		if (Team == ECombatTeam::Player)
		{
			// Party AI 활성화 (이동 + 어그로 기반 공격)
			if (CombatPartyAIComp)
			{
				CombatPartyAIComp->Role = CharacterComp->GetRole();
				CombatPartyAIComp->SetComponentTickEnabled(true);
			}
			// CombatAIActionSelectorComponent 비활성화 (행동 중복 방지)
			if (AIActionSelectorComp)
			{
				AIActionSelectorComp->SetComponentTickEnabled(false);
			}
		}
		else if (Team == ECombatTeam::Enemy)
		{
			// Party AI 불필요
			if (CombatPartyAIComp)
			{
				CombatPartyAIComp->SetComponentTickEnabled(false);
			}
			// CombatAIActionSelectorComponent 비활성화 (EnemyAIController가 공격 담당)
			if (AIActionSelectorComp)
			{
				AIActionSelectorComp->SetComponentTickEnabled(false);
			}
			// 기본 AI 컨트롤러를 EnemyAIController로 교체 (FSM 이동 + 공격)
			if (AController* OldController = GetController())
			{
				OldController->UnPossess();

				if (IsValid(OldController))
				{
					OldController->Destroy();
				}
			}
			if (UWorld* World = GetWorld())
			{
				AEnemyAIController* EnemyAI = World->SpawnActor<AEnemyAIController>();
				if (EnemyAI)
				{
					EnemyAI->Possess(this);
				}
			}
		}
		else
		{
			// Enemy/Neutral제외하고 Party AI 불필요
			if (CombatPartyAIComp)
			{
				CombatPartyAIComp->SetComponentTickEnabled(false);
			}
		}
	}

	// UserWidget 인스턴스를 가져와서 바인딩
	if (HPBarWidgetComponent)
	{
		HPBarWidgetComponent->SetVisibility(false);
		if (UEnemyHPBarWidget* HPWidget = Cast<UEnemyHPBarWidget>(HPBarWidgetComponent->GetUserWidgetObject()))
		{
			if (UHPComponent* MyHPComp = FindComponentByClass<UHPComponent>())
			{
				//HPWidget->BindHPComponent(MyHPComp);
			}
		}
	}
}

void ACombatCharacterActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HPComp)
	{
		HPComp->OnDeath.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ACombatCharacterActor::HandleOnDeath(AActor* Killer, FName ReasonTag)
{
	UE_LOG(LogTemp, Log, TEXT("CombatCharacterActor::HandleOnDeath : %s 사망 (Reason=%s)"),
	*GetName(), *ReasonTag.ToString());

	if (CustomTimeDilation <= 0.06f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Death][TimeDilationReset] Owner=%s PreviousDilation=%.3f"), *GetNameSafe(this), CustomTimeDilation);
		CustomTimeDilation = 1.f;
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (MeshComp->GlobalAnimRateScale <= 0.1f)
		{
			MeshComp->GlobalAnimRateScale = 1.f;
		}
	}

	// 진행 중인 프레젠테이션 취소 (몽타주 중단 + 기존 입력 잠금 해제)
	if (PresentationComp)
	{
		PresentationComp->CancelActivePresentation("Death.Killed", false);
	}

	// 이동 영구 잠금 (액터 파괴 시 LocomotionComponent::EndPlay에서 자동 해제)
	if (LocomotionComp)
	{
		const TJRPGResult<FJRPGHandle> Result = LocomotionComp->AcquireInputLock("Death");
		if (Result.bOk)
		{
			DeathInputLockHandle = Result.Value;
		}
	}

	// 사망 몽타주 재생
	if (DeathMontage)
	{
		// 적 사망시
		if (CharacterComp && CharacterComp->GetTeam() == ECombatTeam::Enemy)
		{
			if (USkeletalMeshComponent* MeshComp = GetMesh())
			{
				if(UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
					GetCapsuleComponent()->SetCollisionProfileName("IgnoreOnlyPawn");

				MeshComp->SetCollisionProfileName("IgnoreOnlyPawn");
				MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				MeshComp->SetPlayRate(1.0f);
				MeshComp->PlayAnimation(DeathMontage, false);

				if (DeathNiagaraEffect)
				{
					UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation
					(
						GetWorld(),
						DeathNiagaraEffect,
						GetActorLocation(),
						FRotator(0,0,0),
						FVector(1.0f),       
						true,                
						true,                
						ENCPoolMethod::None, 
						true
					);
				}

				GetWorld()->GetTimerManager().SetTimer(
					DeathDestoryHandle,
					this,
					&ACombatCharacterActor::DeathDestory,
					DeathMontage->GetPlayLength() + 1.0f,
					false
				);

			}
		}
		// 플레이어, 아군 사망시
		else if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->Montage_Play(DeathMontage, 1.0f);
		}
	}

	// HP Widget Component 제거
	if (HPBarWidgetComponent)
	{
		HPBarWidgetComponent->SetVisibility(false);
	}

	if (TargetGuideLineComp)
	{
		TargetGuideLineComp->ClearAggroTarget();
	}
}

void ACombatCharacterActor::DeathDestory()
{
	if(GetWorld()->GetTimerManager().IsTimerActive(DeathDestoryHandle))
	GetWorld()->GetTimerManager().ClearTimer(DeathDestoryHandle);

	EndPlay(EEndPlayReason::Destroyed);
	this->Destroy();
}

FName ACombatCharacterActor::GetCombatantId() const
{
	return CharacterComp ? CharacterComp->GetCharacterId() : NAME_None;
}

ECombatTeam ACombatCharacterActor::GetCombatTeam() const
{
	return CharacterComp ? CharacterComp->GetTeam() : ECombatTeam::Neutral;
}

bool ACombatCharacterActor::IsPlayerControlledCombatant() const
{
	return IsPlayerControlled();
}

UActorComponent* ACombatCharacterActor::GetOptionalComponentByClass(TSubclassOf<UActorComponent> CompClass) const
{
	return CompClass ? GetComponentByClass(CompClass) : nullptr;
}

void ACombatCharacterActor::ResetEnemyRuntimeForRematch(FName ReasonTag)
{
	if (!CharacterComp || CharacterComp->GetTeam() != ECombatTeam::Enemy)
		return;
	
	if (PresentationComp)
	{
		PresentationComp->CancelActivePresentation(ReasonTag, true);
	}

	if (MotionComp && MotionComp->IsMotionActive())
	{
		MotionComp->CancelCombatMotion(MotionComp->GetMotionState().ActiveHandle, ReasonTag);
	}

	if (LocomotionComp)
	{
		if (DeathInputLockHandle.IsValid())
		{
			LocomotionComp->ReleaseInputLock(DeathInputLockHandle);
			DeathInputLockHandle = FJRPGHandle();
		}
		LocomotionComp->SetMoveInput(FVector2D::ZeroVector);
		LocomotionComp->SetSprint(false);
		LocomotionComp->SetMovementEnabled(true);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	if (HPComp)
	{
		HPComp->RestoreFull(ReasonTag);
	}

	if (ThreatComp)
	{
		ThreatComp->ClearAll();
	}

	if (TargetGuideLineComp)
	{
		TargetGuideLineComp->ClearAggroTarget();
	}

	if (GroggyComp)
	{
		GroggyComp->ResetGauge(ReasonTag);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}

	AEnemyAIController* EnemyAI = Cast<AEnemyAIController>(GetController());
	if (!EnemyAI)
	{
		if (AController* ExistingController = GetController())
		{
			ExistingController->UnPossess();
			ExistingController->Destroy();
		}

		if (UWorld* World = GetWorld())
		{
			EnemyAI = World->SpawnActor<AEnemyAIController>();
			if (EnemyAI)
			{
				EnemyAI->Possess(this);
			}
		}
	}

	if (EnemyAI)
	{
		EnemyAI->ResetForNewBattle();
	}
}


//-----ICameraTargetInterface
FVector ACombatCharacterActor::GetCameraTargetLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 60.f);
}

FRotator ACombatCharacterActor::GetCameraTargetRotation() const
{
	if (AController* C = GetController())
		return C->GetControlRotation();
	return GetActorRotation();
}

