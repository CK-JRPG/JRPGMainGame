#include "CombatTimeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UCombatTimeSubsystem::ApplyDilation(float Value)
{
	if (!GetWorld()) return;
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), FMath::Clamp(Value, 0.001f, 10.f));
}

void UCombatTimeSubsystem::EnterTactical(float DurationRealSec, float Dilation)
{
	if (!GetWorld()) return;

	// 체인 중에는 전술로 덮지 않음(체인이 우선)
	if (bChainActive) return;

	bTacticalActive = true;
	TacticalDilation = Dilation;
	TacticalEndRealTime = GetWorld()->GetRealTimeSeconds() + FMath::Max(0.f, DurationRealSec);

	ApplyDilation(TacticalDilation);
}

void UCombatTimeSubsystem::ExitTactical()
{
	if (!GetWorld()) return;
	bTacticalActive = false;

	// 체인이 없으면 정상속도
	if (!bChainActive)
		ApplyDilation(1.0f);
}

void UCombatTimeSubsystem::EnterChainStop(float Dilation)
{
	if (!GetWorld()) return;

	// 체인 시작 시 전술은 종료(의도적으로 “강제 정지”)
	bTacticalActive = false;

	bChainActive = true;
	ChainDilation = Dilation;
	ApplyDilation(ChainDilation);
}

void UCombatTimeSubsystem::ExitChainStop()
{
	if (!GetWorld()) return;

	bChainActive = false;

	// 체인 종료 후 전술이 남아있다면 전술 속도로 복구(현재는 체인 시작 시 전술 종료하므로 1로)
	if (bTacticalActive) ApplyDilation(TacticalDilation);
	else ApplyDilation(1.0f);
}

void UCombatTimeSubsystem::Tick(float DeltaTime)
{
	if (!GetWorld()) return;

	if (bTacticalActive && !bChainActive)
	{
		const double Now = GetWorld()->GetRealTimeSeconds();
		if (Now >= TacticalEndRealTime)
			ExitTactical();
	}
}