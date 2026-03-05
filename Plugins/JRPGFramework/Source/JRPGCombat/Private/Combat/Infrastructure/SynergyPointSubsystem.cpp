#include "Combat/Infrastructure/SynergyPointSubsystem.h"

FJRPGOpResult USynergyPointSubsystem::ApplyGainEvent(const FJRPGSPGainEvent& Ev)
{
	if (Ev.Amount <= 0)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("SP.InvalidAmount"));

	const int32 Prev = State.Current;
	State.Current = FMath::Clamp(State.Current + Ev.Amount, 0, State.Max);

	if (State.Current != Prev)
	{
		OnSPChanged.Broadcast(State);
	}
	return FJRPGOpResult::Ok();
}

FJRPGOpResult USynergyPointSubsystem::ConsumeForChain(int32 Cost, FName /*ReasonTag*/)
{
	if (Cost <= 0)
		return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("SP.InvalidCost"));

	if (State.Current < Cost)
		return FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("SP.NotEnough"));

	State.Current -= Cost;
	OnSPChanged.Broadcast(State);
	return FJRPGOpResult::Ok();
}