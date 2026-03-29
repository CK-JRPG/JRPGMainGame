#include "Game/PartySetupService.h"

#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Combat/Characters/CombatTransitionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Game/Companion/FieldCompanionSubsystem.h"
#include "Game/JRPGPlayerPawn.h"
#include "Game/JRPGPlayerController.h"

void UPartySetupService::InitializeCombatBridge(UDataTable* CharacterTable,
	TSubclassOf<APlayerController> InCombatControllerClass,
	APawn* PlayerPawn)
{
	if (!IsValid(CharacterTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("PartySetupService : CharacterTable이 설정되지 않음."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;

	UPartySubsystem* PartySubsystem = GI->GetSubsystem<UPartySubsystem>();
	if (!PartySubsystem) return;

	UPartyActorSpawnSubsystem* SpawnSub = World->GetSubsystem<UPartyActorSpawnSubsystem>();
	UCharacterRuntimeSubsystem* CharacterRuntime = GI->GetSubsystem<UCharacterRuntimeSubsystem>();
	UCombatTransitionSubsystem* TransitionSub = World->GetSubsystem<UCombatTransitionSubsystem>();

	if (!SpawnSub || !CharacterRuntime) return;

	// BP_JPRGPlayerController 에서 설정한 전투 전용 컨트롤러 클래스 전달
	if (TransitionSub && InCombatControllerClass)
	{
		TransitionSub->SetCombatControllerClass(InCombatControllerClass);
	}

	const TArray<FName>& CharIds = PartySubsystem->GetPartyIds();
	FName LeaderId = CharIds.Num() > 0 ? CharIds[0] : NAME_None;

	for (const FName& CharId : CharIds)
	{
		FCharacterMappingRow* Row = CharacterTable->FindRow<FCharacterMappingRow>(CharId, TEXT("PartySetupService"));
		if (!Row)
		{
			UE_LOG(LogTemp, Warning, TEXT("PartySetupService : [%s] MappingRow 없음. 스킵."), *CharId.ToString());
			continue;
		}

		if (!Row->CharacterAsset)
		{
			UE_LOG(LogTemp, Warning, TEXT("PartySetupService : [%s] CharacterAsset 없음. 스킵."), *CharId.ToString());
			continue;
		}

		FCharacterSpawnEntry Entry;
		Entry.CharacterID  = CharId;
		Entry.ActorClass   = Row->CombatActorClass;
		Entry.SpawnOffset  = Row->SpawnOffset;
		Entry.FieldPawnClass = Row->FieldPawnClass;
		SpawnSub->RegisterSpawnEntry(Entry);

		// 스냅샷 초기화로 데이터 에셋에서 스탯들 수치 직접 읽어옴
		const FCharacterBaseParams& P = Row->CharacterAsset->BaseParams;
		CharacterRuntime->InitializeSnapshotIfAbsent(CharId, P.MaxHP, P.MaxAP, P.MaxSP);

		UE_LOG(LogTemp, Log, TEXT("PartySetupService : [%s] 등록 완료. HP=%.1f AP=%d SP=%d"),
			*CharId.ToString(), P.MaxHP, P.MaxAP, P.MaxSP);
	}

	// 필드 컴패니언 스폰
	if (UFieldCompanionSubsystem* CompanionSub = World->GetSubsystem<UFieldCompanionSubsystem>())
	{
		if (PlayerPawn)
		{
			CompanionSub->SpawnFieldCompanions(PlayerPawn->GetActorLocation(), LeaderId, SpawnSub->GetSpawnEntryMap());
		}
	}

	// PlayerPawn에 리드 캐릭터 ID 연결
	if (AJRPGPlayerPawn* JRPGPawn = Cast<AJRPGPlayerPawn>(PlayerPawn))
	{
		if (CharIds.Num() > 0 && JRPGPawn->CurrentCharacterId.IsNone())
		{
			JRPGPawn->UpdateCharacter(CharIds[0]);
			UE_LOG(LogTemp, Log, TEXT("PartySetupService : PlayerPawn 리드 캐릭터 → %s"), *CharIds[0].ToString());
		}
	}
}