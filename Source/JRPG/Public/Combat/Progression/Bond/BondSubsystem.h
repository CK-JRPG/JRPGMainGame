// Source/JRPGCombat/Public/Combat/Progression/Bond/BondSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Combat/Progression/Bond/BondTypes.h"
#include "Combat/Progression/Bond/BondSettingsDataAsset.h"
#include "Combat/Progression/Bond/BondDialogueNodeDataAsset.h"
#include "Combat/Progression/Leveling/BondExpBonusProvider.h"// 레벨업 시스템이 조회(인터페이스) :contentReference[oaicite:27]{index=27}


#include "BondSubsystem.generated.h"

USTRUCT()
struct FBondAddRequest
{
	GENERATED_BODY()

	UPROPERTY() EBondSource Source = EBondSource::Walk;
	UPROPERTY() TArray<FName> Participants;
	UPROPERTY() int32 BaseAmount = 0;

	// Context(문서의 Context) + 텔레메트리 sourceTag
	UPROPERTY() FName Context = NAME_None;
	UPROPERTY() FName SourceTag = "Bond.BP.Gained";

	// 텔레메트리 location
	UPROPERTY() 
	FVector WorldLocation = FVector::ZeroVector;
};

UCLASS()
class JRPG_API UBondSubsystem : public UGameInstanceSubsystem, public IBondExpBonusProvider
{
	GENERATED_BODY()

public:
	// ---- Data ----
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBondSettingsDataAsset> Settings = nullptr;
	
	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<UBondDialogueNodeDataAsset>> DialogueNodes;

	// ---- Events (SSOT) :contentReference[oaicite:28]{index=28} ----
	FOnBondPointsGained OnBondPointsGained;
	FOnBondLevelUp OnBondLevelUp;
	FOnBondDialogueUnlocked OnBondDialogueUnlocked;
	FOnBondDialogueCompleted OnBondDialogueCompleted;
	FOnBondExpBonusChanged OnBondExpBonusChanged;

	// ---- API (SSOT) :contentReference[oaicite:29]{index=29} ----
	FBondOp AddBondPoints(const FBondAddRequest& Req);
	FBondOp TryLevelUpBond_Pair(const FBondPairId& BondId);
	FBondOp TryLevelUpBond_Trio(const FBondTrioId& BondId);

	FBondState GetBondState_Pair(const FBondPairId& BondId) const;
	FBondState GetBondState_Trio(const FBondTrioId& BondId) const;

	// “현재 파티 기준” 트리오 레벨/배율 (SSOT) :contentReference[oaicite:30]{index=30}
	int32 GetTrioBondLevelForCurrentParty() const;
	float GetExpBonusMultiplierForCurrentParty() const;

	// 현재 파티(1~3인) 세팅
	FBondOp SetCurrentParty(const TArray<FName>&PartyIds);

	// 유의미 진행(전투/상호작용/지역 변화 등) 알림 → Walk BP 감쇠 해제 :contentReference[oaicite:31]{index=31}
	void NotifySignificantProgress(FName EventTag);

	// 휴식 대화: unlock/complete
	void EvaluateDialogueUnlocks();
	FBondOp CompleteDialogue(FName NodeId);

	// ---- IBondExpBonusProvider ----
	virtual float GetBondExpBonusMultiplier() const override { return CachedExpBonusMultiplier; }

	const TArray<FName>&GetCurrentPartyIds()const { return CurrentPartyIds; }

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	// 저장 상태
	UPROPERTY() TMap<FBondPairId, FBondState> PairStates;
	UPROPERTY() TMap<FBondTrioId, FBondState> TrioStates;

	UPROPERTY() TSet<FName> UnlockedDialogueNodes;
	UPROPERTY() TSet<FName> CompletedDialogueNodes;
	
	UPROPERTY() TArray<FName> CurrentPartyIds; // size=1~3

	UPROPERTY()
	double LastSignificantProgressReal = 0.0;

	// 캐시(파티 기준)
	UPROPERTY() int32 CachedTrioLevel = 1;
	UPROPERTY() float CachedExpBonusMultiplier = 1.0f;

	// helpers
	class UBondSaveGameSubsystem* GetSaveSys() const;
	void LoadFromSave();
	void FlushToSave();

	const UBondSettingsDataAsset* GetSettings() const;

	double NowReal() const;

	FBondState& GetOrInitPairState(const FBondPairId& Id);
	FBondState& GetOrInitTrioState(const FBondTrioId& Id);

	bool ValidateParticipants(const TArray<FName>& P, FName& OutReason) const;
	float ComputeInactivityMul(double Now) const;

	// award core
	FBondOp AddToPairBond(const FBondPairId& Id, EBondSource Source, int32 BaseAmount, const FBondAddRequest& Req);
	FBondOp AddToTrioBond(const FBondTrioId& Id, EBondSource Source, int32 BaseAmount, const FBondAddRequest& Req);

	bool TryAutoLevelUpPair(const FBondPairId& Id);
	bool TryAutoLevelUpTrio(const FBondTrioId& Id);

	// exp bonus cache recompute
	void RecomputePartyTrioLevelAndBonus(bool bBroadcastIfChanged);

	// Leveling link: BondLevelUp 시 EXP 지급 :contentReference[oaicite:32]{index=32}
	void NotifyLevelingSystem_BondLevelUp(bool bIsTrioBond);

	// dialogue helpers
	UBondDialogueNodeDataAsset* FindNode(FName NodeId) const;
	bool AreFlagsSatisfied(const TArray<FName>& Flags) const;
	int32 GetBondLevelForNode(const UBondDialogueNodeDataAsset& Node) const;
};
