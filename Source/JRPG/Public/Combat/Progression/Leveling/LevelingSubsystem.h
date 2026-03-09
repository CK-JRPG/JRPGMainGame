// Source/JRPGCombat/Public/Combat/Progression/Leveling/LevelingSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "LevelingSaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Combat/Progression/Leveling/LevelingTypes.h"
#include "Combat/Progression/Leveling/ExpCurveDataAsset.h"
#include "Combat/Progression/Leveling/ExpSettingsDataAsset.h"
#include "Combat/Progression/Leveling/BondExpBonusProvider.h"

#include "LevelingSubsystem.generated.h"

UCLASS()
class JRPG_API ULevelingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---- Data ----
	UPROPERTY(EditAnywhere)
	TObjectPtr<UExpCurveDataAsset> ExpCurve = nullptr;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UExpSettingsDataAsset> ExpSettings = nullptr;

	// Bond bonus provider (인연 시스템이 구현되면 여기에 주입하거나, 월드에서 구현체를 찾아 연결)
	UPROPERTY(EditAnywhere)
	TScriptInterface<IBondExpBonusProvider> BondBonusProvider;

	// ---- Events (SSOT) :contentReference[oaicite:23]{index=23} ----
	FOnExpGranted OnExpGranted;
	FOnPartyLevelUp OnPartyLevelUp;
	FOnExpBonusMultiplierChanged OnExpBonusMultiplierChanged;
	FOnAreaDiscovered OnAreaDiscovered;
	FOnRestPointDiscovered OnRestPointDiscovered;

	// ---- API (SSOT) :contentReference[oaicite:24]{index=24} ----
	FExpGrantOp GrantExp(const FExpGrantRequest& Req);

	int32 GetPartyLevel() const { return PartyLevel; }
	int32 GetCurrentExp() const { return CurrentExp; }
	int32 GetExpToNext() const;
	float GetBondExpMultiplier() const { return CachedBondMultiplier; }

	// ---- Convenience wrappers (실수 방지) ----
	FExpGrantOp GrantTravelExp(int32 BaseExp, const FGuid& ContextGuid);
	FExpGrantOp GrantAreaDiscoveryExp(FName AreaId);
	FExpGrantOp GrantRestPointDiscoveryExp(FName RestPointId);
	FExpGrantOp GrantExploreRewardExp(const FGuid& ExplorationObjectId);
	FExpGrantOp GrantBondLevelUpExp(bool bIsTrioBond, const FGuid& BondContext);
	FExpGrantOp GrantCombatRewardExp(int32 BaseExp, const FGuid& BattleSessionId);

	// Travel anti-exploit 보조: “최근 N초 의미있는 사건 없으면 지급률 감소” :contentReference[oaicite:25]{index=25}
	void NotifySignificantEvent(FName EventTag);

	// Save Sync API (TravelExpComponent에서 누산값 저장)
	void SetTravelAccumulator(const struct FTravelExpAccumulator& In);
	FTravelExpAccumulator GetTravelAccumulator() const;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	// Save state (SSOT: 파티 공유) :contentReference[oaicite:26]{index=26}
	UPROPERTY()
	int32 PartyLevel = 1;
	UPROPERTY()
	int32 CurrentExp = 0;

	// One-time contexts
	UPROPERTY()
	TSet<FName> DiscoveredAreas;
	UPROPERTY()
	TSet<FName> DiscoveredRestPoints;
	UPROPERTY()
	TSet<FGuid> ClaimedExploreRewards;

	UPROPERTY()
	FTravelExpAccumulator TravelAcc;
	UPROPERTY()
	double LastSignificantEventReal = 0.0;

	// bond multiplier cache
	UPROPERTY()
	float CachedBondMultiplier = 1.0f;

	// helpers
	void LoadFromSave();
	void FlushToSave();

	class ULevelingSaveGameSubsystem* GetSaveSys() const;

	int32 LevelMin() const;
	int32 LevelMax() const;
	int32 ExpToNextFor(int32 Level) const;

	float QueryBondMultiplierAndBroadcastIfChanged();

	// rules
	FExpGrantOp ValidateAndApplyGrant(const FExpGrantRequest& Req, int32 BaseExpResolved);
	bool IsDuplicateByContext(const FExpGrantRequest& Req, FName& OutReason) const;

	float ComputeInactivityMultiplier(double NowReal) const;
};
