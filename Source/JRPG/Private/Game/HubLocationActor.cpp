#include "Game/HubLocationActor.h"
#include "Combat/Characters/CombatTransitionSubsystem.h"
#include "Components/BillboardComponent.h"

AHubLocationActor::AHubLocationActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComp->SetupAttachment(RootComponent);
	
	NiagaraComp->SetAutoActivate(true);
	
#if WITH_EDITORONLY_DATA
	EditorSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
	if (EditorSprite)
	{
		EditorSprite->SetupAttachment(RootComponent);
		EditorSprite->SetHiddenInGame(true);
	}
#endif
}

void AHubLocationActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (UWorld* World = GetWorld())
	{
		if (UCombatTransitionSubsystem* TransitionSub = World->GetSubsystem<UCombatTransitionSubsystem>())
		{
			TransitionSub->RegisterHubLocation(this);
		}
	}
}

void AHubLocationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UCombatTransitionSubsystem* TransitionSub = World->GetSubsystem<UCombatTransitionSubsystem>())
		{
			TransitionSub->UnregisterHubLocation(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}
