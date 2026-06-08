#include "Game/LevelEndManagerActor.h"

#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "Combat/Stats/HPComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/LevelEnd/LevelEndUIWidget.h"

ALevelEndManagerActor::ALevelEndManagerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
	LevelEndWidgetClass = ULevelEndUIWidget::StaticClass();
}

void ALevelEndManagerActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InitialTrackingTimerHandle,
			this,
			&ALevelEndManagerActor::InitializeTrackedEnemies,
			InitialTrackingDelay,
			false);
	}
}

void ALevelEndManagerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindTrackedEnemies();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialTrackingTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ALevelEndManagerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bLevelEndTriggered)
	{
		EnsureLevelEndCursorVisible();
	}
}

void ALevelEndManagerActor::InitializeTrackedEnemies()
{
	UnbindTrackedEnemies();
	TrackedEnemies.Reset();
	RemainingEnemyCount = 0;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, ACombatCharacterActor::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		ACombatCharacterActor* CombatActor = Cast<ACombatCharacterActor>(Actor);
		if (!CombatActor || !CombatActor->CharacterComp || CombatActor->CharacterComp->GetTeam() != ECombatTeam::Enemy)
		{
			continue;
		}

		if (UHPComponent* HP = CombatActor->HPComp)
		{
			if (!HP->IsDead())
			{
				HP->OnDeath.RemoveAll(this);
				HP->OnDeath.AddUObject(this, &ALevelEndManagerActor::HandleEnemyDeath);
				TrackedEnemies.Add(CombatActor);
				++RemainingEnemyCount;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LevelEndManager] Initialized enemies. Tracked=%d Remaining=%d"),
		TrackedEnemies.Num(),
		RemainingEnemyCount);

	if (RemainingEnemyCount <= 0)
	{
		TriggerLevelEnd();
	}
}

void ALevelEndManagerActor::TriggerLevelEnd()
{
	if (bLevelEndTriggered)
	{
		return;
	}

	bLevelEndTriggered = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LevelEndManager] TriggerLevelEnd. WidgetClass=%s"), *GetNameSafe(LevelEndWidgetClass));

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);

	if (!LevelEndWidget && LevelEndWidgetClass)
	{
		LevelEndWidget = PC
			? CreateWidget<ULevelEndUIWidget>(PC, LevelEndWidgetClass)
			: CreateWidget<ULevelEndUIWidget>(World, LevelEndWidgetClass);
	}

	if (LevelEndWidget)
	{
		LevelEndWidget->AddToViewport(500);
		LevelEndWidget->ShowEndScreen();
	}

	ApplyLevelEndInputMode(PC);
	SetActorTickEnabled(true);
}

void ALevelEndManagerActor::ApplyLevelEndInputMode(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	if (LevelEndWidget)
	{
		InputMode.SetWidgetToFocus(LevelEndWidget->TakeWidget());
	}
	PC->SetInputMode(InputMode);

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	EnsureLevelEndCursorVisible();
}

void ALevelEndManagerActor::EnsureLevelEndCursorVisible() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
	}

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;
	PC->DefaultMouseCursor = EMouseCursor::Default;
	PC->CurrentMouseCursor = EMouseCursor::Default;
}

void ALevelEndManagerActor::HandleEnemyDeath(AActor* /*Killer*/, FName /*ReasonTag*/)
{
	if (bLevelEndTriggered)
	{
		return;
	}

	RemainingEnemyCount = FMath::Max(0, RemainingEnemyCount - 1);
	UE_LOG(LogTemp, Log, TEXT("[LevelEndManager] Enemy died. Remaining=%d"), RemainingEnemyCount);

	if (RemainingEnemyCount <= 0)
	{
		TriggerLevelEnd();
	}
}

bool ALevelEndManagerActor::IsEnemyAlive(const ACombatCharacterActor* Enemy) const
{
	if (!IsValid(Enemy))
	{
		return false;
	}

	const UHPComponent* HP = Enemy->HPComp;
	return HP && !HP->IsDead();
}

void ALevelEndManagerActor::UnbindTrackedEnemies()
{
	for (ACombatCharacterActor* Enemy : TrackedEnemies)
	{
		if (Enemy && Enemy->HPComp)
		{
			Enemy->HPComp->OnDeath.RemoveAll(this);
		}
	}
}
