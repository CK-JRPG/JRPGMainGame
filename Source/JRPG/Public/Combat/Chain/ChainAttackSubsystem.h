#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Chain/ChainAttackTypes.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "ChainAttackSubsystem.generated.h"

class UBattleSessionSubsystem;
class UBasicCombatSubsystem;

UCLASS()
class JRPG_API UChainAttackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnChainAttackStarted OnChainAttackStarted;
	FOnChainAttackStepResolved OnChainAttackStepResolved;
	FOnChainAttackEnded OnChainAttackEnded;

	bool IsActive() const { return Snapshot.State == EChainAttackState::Active; }
	const FChainAttackSnapshot& GetSnapshot() const { return Snapshot; }

	bool TryStartChain(AActor* Starter, const FChainAttackConfig& Config);
	FCombatActionResult ExecuteChainBasicAttack(AActor* User, AActor* Target);
	void EndChain(FName ReasonTag);

private:
	UPROPERTY() FChainAttackConfig ActiveConfig;
	UPROPERTY() FChainAttackSnapshot Snapshot;

	UPROPERTY() TArray<TWeakObjectPtr<AActor>> ActiveMembers;

	UBattleSessionSubsystem* GetBattle() const;
	UBasicCombatSubsystem* GetBasicCombat() const;

	bool BuildMemberList(const FChainAttackConfig& Config, AActor* Starter);
	void AdvanceChainActor();
	bool IsValidMember(AActor* Actor)const;
	bool IsPlayerActor(AActor* Actor)const;
};
