#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrinityChainSubsystem.generated.h"

UENUM(BlueprintType)
enum class EChainState : uint8
{
	None,
	Selecting,
	Executing
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChainSelecting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChainExecuting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChainEnded);

UCLASS()
class JRPG_API UTrinityChainSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTrinityChainSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return true; }

	UPROPERTY(BlueprintAssignable) FOnChainSelecting OnChainSelecting;
	UPROPERTY(BlueprintAssignable) FOnChainExecuting OnChainExecuting;
	UPROPERTY(BlueprintAssignable) FOnChainEnded     OnChainEnded;

	UFUNCTION() void StartChain(AActor* Target, bool bAutoSelectAndExecute = false);

	UFUNCTION() void SelectSkillFor(AActor* PartyMember, FName SkillId);
	UFUNCTION() void ConfirmAndExecute();
	UFUNCTION() void CancelChain();

	UFUNCTION() EChainState GetState() const { return State; }
	UFUNCTION() AActor* GetChainTarget() const { return ChainTarget.Get(); }
	UFUNCTION() TArray<AActor*> GetPartyRaw() const;

private:
	EChainState State = EChainState::None;

	TWeakObjectPtr<AActor> ChainTarget;

	UPROPERTY() TArray<TWeakObjectPtr<AActor>> PartyMembers;
	UPROPERTY() TMap<TWeakObjectPtr<AActor>, FName> SelectedSkills;

	struct FExecEntry { TWeakObjectPtr<AActor> User; FName SkillId; };
	TArray<FExecEntry> ExecutionList;
	int32 ExecIndex = 0;

	int32 TP = 0; // 간단한 누적치
	float StepDelay = 0.15f; // 실행 간격(느낌용)
	double NextStepRealTime = 0.0;

	void EnterChainTimeStop();
	void ExitChainTimeStop();

	void AutoSelect();
	void BuildExecutionList();
	void ExecuteStep();

	void StopAllAI();
	void ResumeAllAI();
};
