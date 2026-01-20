#include "CombatCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "JRPG/Combat/Stats/HealthComponent.h"
#include "JRPG/Combat/Stats/APComponent.h"
#include "JRPG/Combat/Skill/SkillComponent.h"
#include "JRPG/Combat/AI/CombatAIComponent.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	Health = CreateDefaultSubobject<UHealthComponent>("Health");
	AP     = CreateDefaultSubobject<UAPComponent>("AP");
	Skills = CreateDefaultSubobject<USkillComponent>("Skills");
	CombatAI = CreateDefaultSubobject<UCombatAIComponent>("CombatAI");
	
	// ====== 카메라 ======
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.0f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bDoCollisionTest = true;
	
	CombatCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	CombatCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	CombatCamera->bUsePawnControlRotation = false;
	
	// 이것들 true면 마우스 돌릴 때 뒷통수만 보게 됨
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	// ====== 캐릭터 이동 컴포넌트 설정 ======
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (bIsPartyMember)
		Tags.AddUnique("PartyMember");
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
