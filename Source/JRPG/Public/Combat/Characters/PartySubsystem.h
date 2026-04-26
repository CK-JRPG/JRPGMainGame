#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Core/PartyProvider.h"
#include "Combat/Items/CombatLevelProvider.h"
#include "PartySubsystem.generated.h"

class UCombatCharacterRegistrySubsystem;

UCLASS()
class JRPG_API UPartySubsystem : public UGameInstanceSubsystem, public IPartyProvider, public IJRPGCombatLevelProvider
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase &Collection) override;

	static constexpr int32 MaxPartySize = 3;

	UFUNCTION(BlueprintCallable, Category="JRPG|Party")
	bool SetPartyIds(const TArray<FName>& InPartyIds, FName ReasonTag);
	UFUNCTION(BlueprintCallable, Category="JRPG|Party")
	bool AddPartyMember(FName CharacterId, FName ReasonTag);
	UFUNCTION(BlueprintCallable, Category="JRPG|Party")
	bool RemovePartyMember(FName CharacterId, FName ReasonTag);
	UFUNCTION(BlueprintCallable, Category="JRPG|Party")
	void ClearParty(FName ReasonTag);
	const TArray<FName> &GetPartyIds() const { return PartyIds; }

	virtual void GetPartyMembers(TArray<AActor*>&OutMembers) const override;

	virtual int32 GetCharacterLevel(const AActor *Character) const override;
	virtual int32 GetPartyLevel() const override;

	void SetRestockKey(FName RestockKey);
	FName GetRestockKey() const {return CurrentRestockKey; }

private:
	UPROPERTY() TArray<FName> PartyIds;
	UPROPERTY() FName CurrentRestockKey = NAME_None;

	UCombatCharacterRegistrySubsystem* GetRegistry() const;

	void LoadFromSave();
	void FlushToSave();

	void PushPartyToBond();
	void PushPartyLevelToShop();
};
