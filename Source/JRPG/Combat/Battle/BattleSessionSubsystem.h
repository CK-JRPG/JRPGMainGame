#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BattleSessionSubsystem.generated.h"

class ABattleZoneActor;

UENUM()
enum class EBattleResult : uint8
{
	Victory,
	Defeat
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleEnded, EBattleResult, Result);

UCLASS()
class JRPG_API UBattleSessionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable) FOnBattleStarted OnBattleStarted;
	UPROPERTY(BlueprintAssignable) FOnBattleEnded   OnBattleEnded;

	UFUNCTION() void StartBattle(
		const FVector& Center,
		float RadiusMeters,
		const TArray<AActor*>& Party,
		const TArray<AActor*>& Enemies,
		AActor* InitialAggroTarget
	);

	UFUNCTION() void EndBattle(EBattleResult Result);

	UFUNCTION() bool IsInBattle() const { return bInBattle; }
	UFUNCTION() ABattleZoneActor* GetZone() const { return Zone; }

	UFUNCTION() FVector GetBattleCenter() const { return BattleCenter; }
	UFUNCTION() float GetBattleRadiusCm() const { return BattleRadiusCm; }

	UFUNCTION() AActor* GetMainEnemy() const { return MainEnemy.Get(); }

	UFUNCTION() TArray<AActor*> GetEnemiesRaw() const;
	UFUNCTION() TArray<AActor*> GetPartyRaw() const;

private:
	bool bInBattle = false;

	FVector BattleCenter = FVector::ZeroVector;
	float BattleRadiusCm = 10000.f;

	TWeakObjectPtr<AActor> MainEnemy;
	TWeakObjectPtr<AActor> InitialAggro;

	UPROPERTY() TObjectPtr<ABattleZoneActor> Zone = nullptr;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> PartyMembers;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> EnemyActors;

	FTimerHandle TargetedUpdateTimer;
	FTimerHandle EndCheckTimer;

	void SetupParticipants();
	void UpdateTargetedFlags();
	void CheckEndCondition();
};
