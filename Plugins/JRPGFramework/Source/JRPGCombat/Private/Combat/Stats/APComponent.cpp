#include "Combat/Stats/APComponent.h"

UAPComponent::UAPComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAPComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentAP = FMath::Clamp(CurrentAP, 0, MaxAP);
	LastRealTime = FPlatformTime::Seconds();
	OnAPChanged.Broadcast(CurrentAP, MaxAP);
}

void UAPComponent::SetFullAP()
{
	CurrentAP = MaxAP;
	OnAPChanged.Broadcast(CurrentAP, MaxAP);
}

FJRPGOpResult UAPComponent::Spend(int32 Cost, FName /*ReasonTag*/)
{
	if (Cost <= 0) return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("AP.InvalidCost"));
	if (CurrentAP < Cost) return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("AP.NotEnough"));

	CurrentAP -= Cost;
	OnAPChanged.Broadcast(CurrentAP, MaxAP);
	return FJRPGOpResult::Ok();
}

FJRPGOpResult UAPComponent::Gain(int32 Amount, FName /*ReasonTag*/)
{
	if (Amount <= 0) return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("AP.InvalidAmount"));

	CurrentAP = FMath::Clamp(CurrentAP + Amount, 0, MaxAP);
	OnAPChanged.Broadcast(CurrentAP, MaxAP);
	return FJRPGOpResult::Ok();
}

void UAPComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableRegen || RegenPerSecond <= 0.f) return;
	if (CurrentAP >= MaxAP) return;

	const double Now = FPlatformTime::Seconds();
	const float RealDelta = (float)FMath::Max(0.0, Now - LastRealTime);
	LastRealTime = Now;

	if (RealDelta <= 0.f) return;

	const int32 Add = (int32)FMath::FloorToInt(RegenPerSecond * RealDelta);
	if (Add <= 0) return;

	CurrentAP = FMath::Clamp(CurrentAP + Add, 0, MaxAP);
	OnAPChanged.Broadcast(CurrentAP, MaxAP);
}