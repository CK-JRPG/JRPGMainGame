#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Time/CombatTimeTypes.h"
#include "CombatTimeSubsystem.generated.h"

/**
 * 전술/체인 공용 시간 SSOT
 * - Handle 기반 요청/해제
 * - 우선순위 스택(Chain이 Tactical 위에 덮을 수 있음)
 * - DurationRealSec 만료 시 자동 해제(영구 슬로모 방지)
 * - BlendIn/Out RealTime 기반
 */
UCLASS()
class JRPGCOMBAT_API UCombatTimeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	FCombatTimeResult RequestTimeMode(const FCombatTimeRequest &Req);
	FJRPGOpResult ReleaseTimeMode(FCombatTimeHandle Handle, FName ReasonTag);

	float GetAppliedTimeScale() const { return AppliedScale; }

	virtual void Initialize(FSubsystemCollectionBase &Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(UCombatTimeSubsystem,STATGROUP_Tickables); }

private:
	struct FEntry
		{
			FCombatTimeHandle Handle;
			FCombatTimeRequest Req;
			double StartReal = 0.0;
			double ExpireReal = 0.0;
		};

	uint64 NextHandle = 1;
	TArray<FEntry> Entries;

	// applied scale (global)
	float AppliedScale = 1.0f;

	// blending
	bool bBlending = false;
	double BlendStartReal = 0.0;
	float BlendDuration = 0.f;
	float BlendFrom = 1.f;
	float BlendTo = 1.f;

	double LastReal = 0.0;

	void RemoveExpired(double NowReal);

	const FEntry* PickWinningEntry() const;
	float DesiredScaleFromEntry(const FEntry *E) const;

	void StartBlend(float From, float To, float DurationSec, double NowReal);
	void ApplyScale(float NewScale);

	void EnsureBackToNormal();
};
