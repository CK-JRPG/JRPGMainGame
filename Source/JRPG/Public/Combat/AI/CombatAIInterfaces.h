
// Source/JRPGCombat/Public/Combat/AI/CombatAIInterfaces.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatAIInterfaces.generated.h"

// ---- SP Provider (AI는 SP를 직접 조작하지 않고, 상태/세팅만 참조하는 용도)
// SP는 전투 중 감소하지 않으며, 체인 종료/전투 종료에서만 초기화 :contentReference[oaicite:4]{index=4}
USTRUCT(BlueprintType)
struct FCombatSPSettingsView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 SPCap = 100;
	UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 SPMaxGainPerSec = 50; // 악용 방지 상한 :contentReference[oaicite:5]{index=5}
	UPROPERTY(EditAnywhere,BlueprintReadOnly) float SameEventCooldownSec = 3.f; // 동일 이벤트 반복 억제 :contentReference[oaicite:6]{index=6}
	UPROPERTY(EditAnywhere,BlueprintReadOnly) float TacticalRoleBonusMultiplier = 1.25f; // 전술 예약 보너스 배율 :contentReference[oaicite:7]{index=7}
};

UINTERFACE(MinimalAPI)
class UCombatSynergyPointProvider : public UInterface
{
	GENERATED_BODY()
};

class ICombatSynergyPointProvider
{
	GENERATED_BODY()

	public:
	virtual int32 GetCurrentSP() const =0;
	virtual int32 GetSPCap() const =0;
	virtual bool IsChainReady() const =0;
	virtual FCombatSPSettingsView GetSettingsView() const =0;
};

// ---- Groggy/Break Gauge Provider (AI가 Stun/Rising 판정용으로만 사용)
// 그로기 문서: Break>=Max -> Stun, Stun 만료 후 Rising, Rising 후 Normal :contentReference[oaicite:8]{index=8}
UENUM(BlueprintType)
enum class EGroggyPhase : uint8
{
	Normal,
	Stunned,
	Rising,
};

UINTERFACE(MinimalAPI)
class UCombatGroggyProvider :public UInterface
{
	GENERATED_BODY()
};

class ICombatGroggyProvider
{
	GENERATED_BODY()

public:
	virtual EGroggyPhase GetGroggyPhase() const = 0;
	virtual float GetBreakRatio01() const = 0;// 0..1 (BreakValue/BreakMax)
};

// ---- Chain Flow Provider (체인 시퀀스 Active 여부)
// 체인은 상위 전투 기능(시간/입력/연출 레이어 재배치) :contentReference[oaicite:9]{index=9}
UINTERFACE(MinimalAPI)
class UCombatChainFlowProvider : public UInterface
{
	GENERATED_BODY()
};

class ICombatChainFlowProvider
{
	GENERATED_BODY()

public:
	virtual bool IsChainSequenceActive() const =0;
};