#include "Combat/Characters/CombatCharacterActor.h"

#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Skills/SkillComponent.h"
#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"
#include "Combat/AI/CombatAIActionSelectorComponent.h"
#include "Combat/AI/CombatCharacterActorAIController.h"
#include "Combat/Battle/EnemyEncounterComponent.h"
#include "Combat/Items/CombatItemComponent.h"
#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Motion/CombatMotionComponent.h"
#include "Combat/Movement/LocomotionComponent.h"
#include "Combat/Movement/JRPGCharacterMovementComponent.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/Stats/CombatStatsComponent.h"
#include "Combat/Session/CombatZoneTrackerComponent.h"

#include "UI/Combat/EnemyHPBarWidget.h"
#include "Combat/Stats/HPComponent.h"


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
	MotionComp = CreateDefaultSubobject<UCombatMotionComponent>(TEXT("CombatMotionComponent"));
	LocomotionComp = CreateDefaultSubobject<ULocomotionComponent>(TEXT("LocomotionComponent"));
	EnemyEncounterComp = CreateDefaultSubobject<UEnemyEncounterComponent>(TEXT("EnemyEncounterComponent"));
	ZoneTrackerComp = CreateDefaultSubobject<UCombatZoneTrackerComponent>(TEXT("CombatZoneTracker"));

	// HPBarWidget
	HPBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComponent"));
	HPBarWidgetComponent->SetupAttachment(RootComponent);

	HPBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	HPBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComponent->SetDrawSize(FVector2D(150.f, 20.f));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ACombatCharacterActor::BeginPlay()
{
	Super::BeginPlay();
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

float ACombatCharacterActor::GetCameraTargetArmLength() const
{
	return CombatArmLength;
}

