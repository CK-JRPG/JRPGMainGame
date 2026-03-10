// Source/JRPGCombat/Private/Combat/Items/EconomySubsystem.cpp
#include "Combat/Items/EconomySubsystem.h"

void UEconomySubsystem::AddGold(int32 Amount, FName /*SourceTag*/)
{
	if (Amount <= 0) return;
	const int32 Before = Gold;
	Gold += Amount;
	OnGoldChanged.Broadcast(Before, Gold);
}

FItemOp UEconomySubsystem::TrySpendGold(int32 Amount, FName /*ReasonTag*/)
{
	if (Amount <= 0) return FItemOp::Fail("Reject.InvalidPrice");
	if (Gold < Amount) return FItemOp::Fail("Reject.NotEnoughCurrency");

	const int32 Before = Gold;
	Gold -= Amount;
	OnGoldChanged.Broadcast(Before, Gold);
	return FItemOp::Ok();
}