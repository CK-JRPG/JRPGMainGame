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
	
	
	EnsureDefaultPartyFromTable();
	InitallizeCombatBridge();
}

//TODO : 이 부분은 나중에 게임 로드 시스템에서 관리하는게 맞는거 같았지만, 지금은 테스트 때문에 넣어둠. 또한, 각 Pawn의 컨트롤러 변경(빙의)관련해서도 고려해야함.

void AJRPGPlayerController::EnsureDefaultPartyFromTable()
{
	if (!IsValid(CharacterTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bridge : CharacterTable이 설정되지 않음. DefaultPartyIds도 무시됨."));
		return;
	}
	
	UPartySubsystem * PartySys = GetGameInstance()->GetSubsystem<UPartySubsystem>();
	if (!PartySys)
		return;
	
	if (PartySys->GetPartyIds().Num() == 3)
	{
		UE_LOG(LogTemp, Log, TEXT("Bridge : 이미 파티 데이터가 존재하기 때문에 자동으로 초기화하지 않음."));
		return;
	}
	
	TArray<FName> AllRowNames = CharacterTable->GetRowNames();
	if (AllRowNames.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bridge: CharacterTable에 Row가 %d개밖에 없음. 최소 3개 필요."), AllRowNames.Num());
		return;
	}

	// DefaultPartyIds가 에디터에서 지정되어 있으면 그걸 사용하고 없으면 테이블 첫 3개 가져와서 사용해야함.
	TArray<FName> PartyToSet;
	if (DefaultPartyIds.Num() == 3)
	{
		PartyToSet = DefaultPartyIds;
		UE_LOG(LogTemp, Log, TEXT("Bridge: DefaultPartyIds 사용."));
	}
	else
	{
		PartyToSet = { AllRowNames[0], AllRowNames[1], AllRowNames[2] };
		UE_LOG(LogTemp, Log, TEXT("Bridge: CharacterTable 첫 3개 Row를 기본 파티로 사용."));
	}

	const bool bOk = PartySys->SetPartyIds(PartyToSet, "Init.DefaultParty");
	if (bOk)
	{
		UE_LOG(LogTemp, Log, TEXT("Bridge: 기본 파티 설정 완료 [%s, %s, %s]"),
			*PartyToSet[0].ToString(), *PartyToSet[1].ToString(), *PartyToSet[2].ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Bridge: 기본 파티 설정 실패."));
	}}

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
		if (!FoundDef)
		{
			UE_LOG(LogTemp, Warning, TEXT("Bridge : CharId[%s]에 해당하는 DataAsset을 CharacterTable에서 찾지 못함."), *CharId.ToString());
			continue;
		}

		ACombatCharacterActor* BackCombatCharacter = GetWorld()->SpawnActorDeferred<ACombatCharacterActor>(
			ACombatCharacterActor::StaticClass(),  
			FTransform::Identity,
			nullptr, 
			nullptr, 
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		
		
		if (!IsValid(BackCombatCharacter))
		{
			UE_LOG(LogTemp, Error,
				TEXT("Bridge: CombatCharacterActor 스폰 실패 (%s)"), *CharId.ToString());
			continue;
		}
		
		if (IsValid(BackCombatCharacter->CharacterComp))
		{
			BackCombatCharacter->CharacterComp->CharacterDef = FoundDef;
		}
		
		BackCombatCharacter->FinishSpawning(FTransform::Identity);
		BackCombatCharacter->SetActorHiddenInGame(true);
		BackCombatCharacter->SetActorEnableCollision(false);

		UE_LOG(LogTemp, Log,
		   TEXT("Bridge: 파티 데이터 등록 완료 (%s)"), *CharId.ToString());
		
	}
	
	//연결-데이터테이블에서 첫번쨰로
	if (AJRPGPlayerPawn* PlayerPawn = Cast<AJRPGPlayerPawn>(GetPawn()))
	{
		const TArray<FName>& CharIds = PartySubsystem->GetPartyIds();
		if (CharIds.Num() > 0 && PlayerPawn->CurrentCharacterId.IsNone())
		{
			PlayerPawn->UpdateCharacter(CharIds[0]);
			UE_LOG(LogTemp, Log, TEXT("Bridge: PlayerPawn 리드 캐릭터 설정 → %s"), *CharIds[0].ToString());
		}
	}
}

UCombatCharacterDataAsset* AJRPGPlayerController::FindCharacterDefById(FName CharId) const
{
	if (!IsValid(CharacterTable))
		return nullptr;
	
	FCharacterMappingRow* Row = CharacterTable->FindRow<FCharacterMappingRow>(CharId, TEXT(" "));
	return Row ? Row->CharacterAsset : nullptr;
}



//------Input------

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



