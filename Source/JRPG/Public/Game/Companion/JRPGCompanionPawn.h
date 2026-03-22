#pragma once

#include "CoreMinimal.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "GameFramework/Character.h"
#include "JRPGCompanionPawn.generated.h"

class ULocomotionComponent;
//class UCombatZoneTrackerComponent;
class ACombatCharacterActor;

UCLASS()
class JRPG_API AJRPGCompanionPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AJRPGCompanionPawn(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, Category="JRPG")
	TObjectPtr<ULocomotionComponent> Locomotion;

	//UPROPERTY(VisibleAnywhere, Category="JRPG")
	//TObjectPtr<UCombatZoneTrackerComponent> ZoneTracker;
	
	// 캐릭터 전투 데이터 식별용 ID
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "JRPG|Combat")
	FName CurrentCharacterId;
	
	UFUNCTION(BlueprintCallable, Category = "JRPG|Combat")
	void UpdateCharacter(FName NewCharId);
	
	UFUNCTION(BlueprintCallable, Category = "JRPG|Combat")
	ACombatCharacterActor* GetCombatCharData() const;
};