#include "Game/HubActor.h"

#include "Game/HubSubsystem.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"

AHubActor::AHubActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComp->SetupAttachment(RootComponent);
	NiagaraComp->SetAutoActivate(true);
	
	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractTriggerSphere"));
	TriggerSphere->SetSphereRadius(InteractRadius);
	TriggerSphere->SetupAttachment(RootComponent);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);
}

void AHubActor::BeginPlay()
{
	Super::BeginPlay();
	
	TriggerSphere->SetSphereRadius(InteractRadius);
	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AHubActor::OnTriggerBeginOverlap);
	TriggerSphere->OnComponentEndOverlap.AddDynamic(this, &AHubActor::OnTriggerEndOverlap);
	
	// 허브 서브시스템에 등록
	if (UHubSubsystem* HubSub = GetWorld()->GetSubsystem<UHubSubsystem>())
	{
		HubSub->RegisterHub(this);
	}
}

void AHubActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHubSubsystem* HubSub = GetWorld()->GetSubsystem<UHubSubsystem>())
	{
		HubSub->UnregisterHub(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AHubActor::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	// 플레이어 캐릭터만 허용
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled())
		return;

	if (UHubSubsystem* HubSub = GetWorld()->GetSubsystem<UHubSubsystem>())
	{
		HubSub->SetFocusedHub(this);
	}
}

void AHubActor::OnTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled())
		return;

	if (UHubSubsystem* HubSub = GetWorld()->GetSubsystem<UHubSubsystem>())
	{
		HubSub->ClearFocusedHub(this);
	}
}

FString AHubActor::GetInteractText() const
{
	return InteractText;
}