// Source/JRPGCombat/Public/Combat/Items/CombatItemExecutionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Items/CombatItemTypes.h"
#include "CombatItemExecutionSubsystem.generated.h"

class UCombatUsableItemDataAsset;

UCLASS()
class JRPG_API UCombatItemExecutionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnCombatItemUsed OnCombatItemUsed;

	FCombatItemUseResult ExecuteUse(const FCombatItemUseRequest& Request);

private:
	bool IsAliveCombatant(AActor* Actor) const;
	bool IsSameTeam(AActor* A, AActor* B) const;
	bool IsEnemyTeam(AActor* A, AActor* B) const;

	bool ValidateTargets(
		AActor* User,
		const UCombatUsableItemDataAsset& ItemDef,
		const TArray<AActor*>& Targets,
		FName& OutReason) const;

	bool WouldAnyEffectApply(
		AActor* User,
		const UCombatUsableItemDataAsset& ItemDef,
		const TArray<AActor*>& Targets) const;
};
