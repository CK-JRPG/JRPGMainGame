#pragma once
#include "CoreMinimal.h"
#include "Combat/Characters/CharacterRuntimeStateTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CharacterRuntimeSubsystem.generated.h"

// UI 업데이트용 델리게이트 선언
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCharacterHPChanged, FName /*CharacterID*/, float /*NewHP*/, float /*MaxHP*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCharacterAPChanged, FName /*CharacterID*/, int32 /*NewAP*/, int32 /*MaxAP*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCharacterSPChanged, FName /*CharacterID*/, int32 /*NewSP*/, int32 /*MaxSP*/);

UCLASS()
class JRPG_API UCharacterRuntimeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	FOnCharacterHPChanged OnHPChanged;
	FOnCharacterAPChanged OnAPChanged;
	FOnCharacterSPChanged OnSPChanged;

	bool CommitState(const FName& CharacterID, const FCharacterRuntimeState& State);
	bool CommitStates(const TMap<FName, FCharacterRuntimeState>& States);
	void InitializeSnapshotIfAbsent(const FName& CharacterID, float MaxHP, int32 MaxAP, int32 MaxSP);
	void RecoverPartyFromWipe(const TArray<FName>& ActivePartyIds, float HPRecoverRatio = 0.2f, float APRecoverRatio = 0.3f);
	bool RecoverPartyAfterVictory(const TArray<FName>& ActivePartyIds, float HPRecoverRatio = 0.05f);

	void ModifyHP(const FName& CharacterID, float Delta);
	void ModifyAP(const FName& CharacterID, int32 Delta);
	void ModifySP(const FName& CharacterID, int32 Delta);
	
	const FCharacterRuntimeState* GetState(const FName& CharacterID) const;
	const FCharacterResourceSnapshot* GetSnapshot(const FName& CharacterID) const { return GetState(CharacterID); }
	bool HasSnapshot(const FName& CharacterID) const;
	
private:
	UPROPERTY()
	TMap<FName, FCharacterRuntimeState> RuntimeStateMap;
};
