#include "Combat/Characters/PartySubsystem.h"
#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"
#include "Combat/Characters/PartySaveGameSubsystem.h"

#if __has_include("Combat/Progression/Leveling/LevelingSubsystem.h")
	#include "Combat/Progression/Leveling/LevelingSubsystem.h"
	#define JRPG_HAS_LEVELING 1
#else
	#define JRPG_HAS_LEVELING 0
#endif

#if __has_include("Combat/Progression/Bond/BondSubsystem.h")
	#include "Combat/Progression/Bond/BondSubsystem.h"
	#define JRPG_HAS_BOND 1
#else
	#define JRPG_HAS_BOND 0
#endif

#if __has_include("Combat/Shop/ShopSubsystem.h")
	#include "Combat/Shop/ShopSubsystem.h"
	#define JRPG_HAS_SHOP 1
#else
	#define JRPG_HAS_SHOP 0
#endif

UCombatCharacterRegistrySubsystem* UPartySubsystem::GetRegistry() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UCombatCharacterRegistrySubsystem>() : nullptr;
}

void UPartySubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);
	LoadFromSave();
	
	if (!PartyIds.IsEmpty())
	{
		PushPartyToBond();
		PushPartyLevelToShop();
	}
}

void UPartySubsystem::LoadFromSave()
{
	if (UPartySaveGameSubsystem*SaveSys =GetGameInstance()->GetSubsystem<UPartySaveGameSubsystem>())
	{
		SaveSys->LoadOrCreate();
		if (UPartySaveGame *S = SaveSys->GetSave())
		{
			if (S->PartyIds.Num() > 0 && S->PartyIds.Num() <= MaxPartySize)
			{
				TSet<FName> UniqueIds;
				bool bValid = true;
				for (const FName& Id : S->PartyIds)
				{
					if (Id.IsNone() || UniqueIds.Contains(Id))
					{
						bValid = false;
						break;
					}
					UniqueIds.Add(Id);
				}

				if (bValid)
				{
					PartyIds = S->PartyIds;
				}
			}
			CurrentRestockKey = S->RestockKey;
		}
	}
}

void UPartySubsystem::FlushToSave()
{
	if (UPartySaveGameSubsystem *SaveSys =GetGameInstance()->GetSubsystem<UPartySaveGameSubsystem>())
	{
		if (UPartySaveGame *S =SaveSys->GetSave())
		{
			S->PartyIds = PartyIds;
			S->RestockKey = CurrentRestockKey;
			SaveSys->MarkDirty();
		}
	}
}

bool UPartySubsystem::SetPartyIds(const TArray<FName>& InPartyIds, FName)
{
	if (InPartyIds.Num() < 1 || InPartyIds.Num() > MaxPartySize)
		return false;

	TSet<FName> S;
	
	for (const FName &Id : InPartyIds)
	{
		if (Id.IsNone())
			return false;
		S.Add(Id);
	}
	
	if (S.Num() != InPartyIds.Num())
		return false;

	PartyIds = InPartyIds;
	PushPartyToBond();
	PushPartyLevelToShop();
	FlushToSave();
	return true;
}

bool UPartySubsystem::AddPartyMember(FName CharacterId, FName ReasonTag)
{
	if (CharacterId.IsNone())
	{
		return false;
	}

	TArray<FName> NextPartyIds = PartyIds;
	if (NextPartyIds.Contains(CharacterId))
	{
		return true;
	}

	if (NextPartyIds.Num() >= MaxPartySize)
	{
		return false;
	}

	NextPartyIds.Add(CharacterId);
	return SetPartyIds(NextPartyIds, ReasonTag);
}

bool UPartySubsystem::RemovePartyMember(FName CharacterId, FName ReasonTag)
{
	if (CharacterId.IsNone())
	{
		return false;
	}

	TArray<FName> NextPartyIds = PartyIds;
	if (!NextPartyIds.RemoveSingle(CharacterId))
	{
		return false;
	}

	if (NextPartyIds.IsEmpty())
	{
		return false;
	}

	return SetPartyIds(NextPartyIds, ReasonTag);
}

void UPartySubsystem::ClearParty(FName)
{
	PartyIds.Reset();
	PushPartyToBond();
	PushPartyLevelToShop();
	FlushToSave();
}

void UPartySubsystem::GetPartyMembers(TArray<AActor*>&OutMembers)const
{
	OutMembers.Reset();
	auto * Reg = GetRegistry();
	
	if (!Reg) 
		return;
	
	for (const FName &Id :PartyIds)
		if (AActor*A =Reg->FindById(Id)) 
			OutMembers.Add(A);
}

int32 UPartySubsystem::GetCharacterLevel(const AActor*) const
{
	return GetPartyLevel();
}

int32 UPartySubsystem::GetPartyLevel() const
{
#if JRPG_HAS_LEVELING
	if (ULevelingSubsystem *L =GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
		return L->GetPartyLevel();
#endif
	return 1;
}

void UPartySubsystem::SetRestockKey(FName RestockKey)
{
	CurrentRestockKey = RestockKey;
#if JRPG_HAS_SHOP
	
#endif
	FlushToSave();
}

void UPartySubsystem::PushPartyToBond()
{
#if JRPG_HAS_BOND
	if (UBondSubsystem* Bond = GetGameInstance()->GetSubsystem<UBondSubsystem>())
	{
		if (PartyIds.Num() >= 1 && PartyIds.Num() <= MaxPartySize)
			Bond->SetCurrentParty(PartyIds);
	}
#endif
}

void UPartySubsystem::PushPartyLevelToShop()
{
#if JRPG_HAS_SHOP
	
#endif
}
