#include "Game/JRPGPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
#include "Combat/Characters/PartySubsystem.h"

#include "Game/JRPGPlayerPawn.h"
#include "Combat/Movement/LocomotionComponent.h"

void AJRPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMC_Default)
			{
				Subsys->AddMappingContext(IMC_Default, 0);
			}
		}
	}
	
	InitallizeCombatBridge();
}

void AJRPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AJRPGPlayerController::OnMove);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &AJRPGPlayerController::OnMove);
	}

	if (IA_Sprint)
	{
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AJRPGPlayerController::OnSprintStarted);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AJRPGPlayerController::OnSprintCompleted);
	}
	
	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AJRPGPlayerController::OnLook);
	}
}

void AJRPGPlayerController::OnMove(const FInputActionValue& Value)
{
	const FVector2D Move = Value.Get<FVector2D>();

	if (AJRPGPlayerPawn* P = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		if (P->Locomotion)
		{
			P->Locomotion->SetMoveInput(Move);
		}
	}
}

void AJRPGPlayerController::OnSprintStarted(const FInputActionValue& /*Value*/)
{
	if (AJRPGPlayerPawn* P = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		if (P->Locomotion) P->Locomotion->SetSprint(true);
	}
}

void AJRPGPlayerController::OnSprintCompleted(const FInputActionValue& /*Value*/)
{
	if (AJRPGPlayerPawn* P = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		if (P->Locomotion) P->Locomotion->SetSprint(false);
	}
}

void AJRPGPlayerController::OnLook(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if (LookAxisVector.X != 0.0f)
	{
		AddYawInput(LookAxisVector.X);
	}
	
	if (LookAxisVector.Y != 0.0f)
	{
		AddPitchInput(LookAxisVector.Y);
	}
}

void AJRPGPlayerController::InitallizeCombatBridge()
{
	UPartySubsystem* PartySubsystem = GetGameInstance()->GetSubsystem<UPartySubsystem>();
	UCombatCharacterRegistrySubsystem* Registry = GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>();
	
	if (!PartySubsystem || !Registry) 
		return;
	
	
		UE_LOG(LogTemp, Error, TEXT("Bridge : 현재 파티원 수 = %d"), PartySubsystem->GetPartyIds().Num());
		

	
	
	// 현재 PartySub에 등록된 파티 ID가지고 와서 레지스트리에 있는지 검사하고서 
	// 이후 없으면 스폰해서 레지스트리에 일단 임시로 등록함.
	for (const FName& CharId : PartySubsystem->GetPartyIds())
	{
		if (IsValid(Registry->FindById(CharId))) 
		{
			UE_LOG(LogTemp, Log, TEXT("Bridge : %s는 이미 런타임에 존재함. 스폰 생략."), *CharId.ToString());
			continue;
		}
		UCombatCharacterDataAsset* FoundDef = FindCharacterDefById(CharId);
		if (FoundDef)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
			ACombatCharacterActor* BackCombatCharacter = GetWorld()->SpawnActor<ACombatCharacterActor>(
				ACombatCharacterActor::StaticClass(), Params);
			
			if (IsValid(BackCombatCharacter) && IsValid(BackCombatCharacter->CharacterComp))
			{
				BackCombatCharacter->CharacterComp->CharacterDef = FoundDef;
				BackCombatCharacter->CharacterComp->InitializeFromDef();
				
				BackCombatCharacter->SetActorHiddenInGame(true);
				BackCombatCharacter->SetActorEnableCollision(false);
			
				UE_LOG(LogTemp, Log, TEXT("Bridge : 파티 데이터 등록 완료 (%s)"), *CharId.ToString());
			}
		}
	}
		
}

UCombatCharacterDataAsset* AJRPGPlayerController::FindCharacterDefById(FName CharId) const
{
	if (IsValid(CharacterTable)) 
		return nullptr;
	
	FCharacterMappingRow* Row = CharacterTable->FindRow<FCharacterMappingRow>(CharId, TEXT(" "));
	return Row ? Row->CharacterAsset : nullptr;
}


