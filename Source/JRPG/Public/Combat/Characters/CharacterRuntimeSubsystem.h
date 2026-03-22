#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CharacterRuntimeSubsystem.generated.h"

class ACombatCharacterActor;

USTRUCT()
struct FCharacterResourceSnapshot
{
	GENERATED_BODY()

	UPROPERTY() float HP    = -1.f;  // -1 = 스냅샷 없음 (첫 인카운터)
	UPROPERTY() float MaxHP = 100.f;
	UPROPERTY() int32 AP    = 0;
	UPROPERTY() int32 MaxAP = 10;
	UPROPERTY() int32 SP    = 0;
	UPROPERTY() int32 MaxSP = 100;
	UPROPERTY() FVector WorldLocation = FVector::ZeroVector;
	UPROPERTY() FRotator WorldRotation = FRotator::ZeroRotator;
	UPROPERTY() bool bHasTransformSnapshot = false;

	bool IsValid() const { return HP >= 0.f; }
};

UCLASS()
class JRPG_API UCharacterRuntimeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	void SaveSnapshot(const FName& CharacterID, ACombatCharacterActor* Actor);
	void RestoreSnapshot(const FName& CharacterID, ACombatCharacterActor* Actor);
	void InitializeSnapshotIfAbsent(const FName& CharacterID, float MaxHP, int32 MaxAP, int32 MaxSP);
	void RecoverPartyFromWipe(float HPRecoverRatio = 0.2f, float APRecoverRatio = 0.3f);
	
	void ModifyHP(const FName& CharacterID, float Delta);
	void ModifyAP(const FName& CharacterID, int32 Delta);
	void ModifySP(const FName& CharacterID, int32 Delta);
	
	const FCharacterResourceSnapshot* GetSnapshot(const FName& CharacterID) const;
	bool HasSnapshot(const FName& CharacterID) const;
	
private:
	UPROPERTY()
	TMap<FName, FCharacterResourceSnapshot> SnapshotMap;
};
