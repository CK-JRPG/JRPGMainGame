#pragma once

#include"CoreMinimal.h"
#include"Components/ActorComponent.h"
#include"GameplayTagContainer.h"
#include"Combat/Groggy/GroggyTypes.h"
#include"Combat/Groggy/GroggySettingsDataAsset.h"
#include"GroggyComponent.generated.h"

/**
 * UGroggyComponent
 * - Break 게이지 누적/감쇠/임계判定의 단일 권위(SSOT) :contentReference[oaicite:18]{index=18}
 * - Stun/Rising은 Status로 관리하되(있으면), 없을 경우 내부 타이머로 동작(백업)
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UGroggyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGroggyComponent();

	// ===== Settings =====
	UPROPERTY(EditAnywhere,Category="Groggy|Settings")
	FName EnemyTypeId = NAME_None;

	UPROPERTY(EditAnywhere,Category="Groggy|Settings")
	TObjectPtr<UGroggySettingsDataAsset> SettingsAsset;

	// 없으면 이 값 사용(프로토타입/테스트용)
	UPROPERTY(EditAnywhere, Category="Groggy|Settings")
	FGroggySettings FallbackSettings;

	// Status 연동(프로젝트의 StatusId 네이밍에 맞게 수정)
	UPROPERTY(EditAnywhere, Category="Groggy|Status")
	FName StunStatusId =TEXT("Stun");

	UPROPERTY(EditAnywhere, Category="Groggy|Status")
	FName RisingStatusId = TEXT("Rising");

	// (선택) 스턴 중 받는 피해 증가 상태 :contentReference[oaicite:19]{index=19}
	UPROPERTY(EditAnywhere, Category="Groggy|Status")
	FName GroggyVulnerableStatusId = TEXT("GroggyVulnerable");

	// ===== Runtime State =====
	UFUNCTION() EGroggyPhase GetPhase() const { return Phase; }
	UFUNCTION() float GetBreakValue() const { return BreakValue; }
	UFUNCTION() float GetBreakMax() const { return CachedSettings.BreakMax; }
	UFUNCTION() float GetBreakRatio() const { return (CachedSettings.BreakMax<=0.f) ? 0.f : (BreakValue/CachedSettings.BreakMax); }

	UFUNCTION()FGroggySnapshot GetSnapshot() const;

	// 스킬 ApplyEffects 단계에서 호출 고정:contentReference[oaicite:20]{index=20}
	UFUNCTION()
	bool AddBreak(AActor*SourceActor, float BreakAmountRaw, const FGameplayTagContainer& ContextTags);

	// 강제 전환(디버그/특수 스크립트)
	UFUNCTION() void ForceEnterStun();
	UFUNCTION() void ForceExitToNormal();

	// Events
	FOnGroggyPhaseChanged OnGroggyPhaseChanged;
	FOnBreakValueChanged OnBreakValueChanged;

protected:
	virtual void BeginPlay()override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // Phase
    UPROPERTY(VisibleAnywhere) EGroggyPhase Phase = EGroggyPhase::Normal;

    // Gauge
    UPROPERTY(VisibleAnywhere) float BreakValue = 0.f;

    // RealTime tracking
    double LastBreakInputReal = 0.0;

    // Cached settings (resolved from SettingsAsset + EnemyTypeId, else fallback)
    FGroggySettings CachedSettings;

    // Status integration
    TScriptInterface<ICombatStatusAccess> StatusAccess;

    // Backup timers when Status system is not present
    bool bUseInternalTimers = true;
    double PhaseEndReal = 0.0;// Stun/Rising end time if internal mode

    void ResolveSettings();
    void ResolveStatusAccess();

    // Core flow
    void ApplyDecayIfNeeded(double NowReal,float DeltaTime);
    void EnterStun_Internal(const FGameplayTag& ReasonTag);
    void EnterRising_Internal(const FGameplayTag& ReasonTag);
    void EnterNormal_Internal(const FGameplayTag& ReasonTag);

    void SetPhase(EGroggyPhase NewPhase,const FGameplayTag& ReasonTag);

    // Helper
    double NowReal()const;
};