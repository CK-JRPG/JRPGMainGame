#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"

#include "Combat/Progression/Leveling/LevelingSubsystem.h"
#include "Combat/Progression/Bond/BondSubsystem.h"
#include "Combat/Shop/ShopSubsystem.h"

UCombatCharacterRegistrySubsystem* UPartySubsystem::GetRegistry() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>() : nullptr;
}

bool UPartySubsystem::SetPartyIds(const TArray<FName>& Party3, FName /*ReasonTag*/)
{
	if (Party3.Num() != 3) return false;

	TSet<FName> S;
	for (const FName& Id :Party3)
	{
		if (Id.IsNone()) return false;
		S.Add(Id);
	}
	if (S.Num() != 3) return false;

	PartyIds = Party3;

	PushPartyToBond();
	PushPartyLevelToShop();

	return true;
}

void UPartySubsystem::GetPartyMembers(TArray<AActor*>& OutMembers) const
{
	OutMembers.Reset();
	UCombatCharacterRegistrySubsystem* Reg = GetRegistry();
	if (!Reg) return;

	for (const FName &Id : PartyIds)
	{
		if (AActor *A = Reg->FindById(Id))
			OutMembers.Add(A);
	}
}

int32 UPartySubsystem::GetCharacterLevel(const AActor* /*Character*/) const
{
	// 파티 공유 레벨 SSOT
	return GetPartyLevel();
}

int32 UPartySubsystem::GetPartyLevel() const
{
	if (ULevelingSubsystem* L = GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
		return L->GetPartyLevel();
	return 1;
}

void UPartySubsystem::SetRestockKey(FName RestockKey)
{
	CurrentRestockKey = RestockKey;

	// 상점에 restock key 주입
	if (UShopSubsystem* Shop = GetGameInstance()->GetSubsystem<UShopSubsystem>())
	{
		Shop->SetCurrentRestockKey(RestockKey);
	}
}

void UPartySubsystem::PushPartyToBond()
{
	if (UBondSubsystem* Bond = GetGameInstance()->GetSubsystem<UBondSubsystem>())
	{
		Bond->SetCurrentParty(PartyIds);
	}
}

void UPartySubsystem::PushPartyLevelToShop()
{
	if (UShopSubsystem* Shop = GetGameInstance()->GetSubsystem<UShopSubsystem>())
	{
		Shop->SetPartyLevel(GetPartyLevel());
	}
}