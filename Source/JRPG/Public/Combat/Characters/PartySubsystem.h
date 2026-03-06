#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Core/PartyProvider.h"
#include "Combat/Items/CombatLevelProvider.h"
#include "PartySubsystem.generated.h"

class UCombatCharacterRegistrySubsystem;

UCLASS()
class JRPG_API UPartySubsystem : public UGameInstanceSubsystem, public IPartyProvider, public ICombatLevelProvider
{
	GENERATED_BODY()

public:
	// 파티 3인 SSOT
	bool SetPartyIds(const TArray<FName>& Party3, FName ReasonTag);
	const TArray<FName>& GetPartyIds() const { return PartyIds; }

	// IPartyProvider
	virtual void GetPartyMembers(TArray<AActor*>& OutMembers) const override;

	// ICombatLevelProvider (파티 공유 레벨)
	virtual int32 GetCharacterLevel(const AActor* Character) const override;
	virtual int32 GetPartyLevel() const override;

	// 상점/리스톡 키(챕터) 주입을 한 곳에서 관리하고 싶으면 여기서 처리
	void SetRestockKey(FName RestockKey);

private:
	UPROPERTY() TArray<FName> PartyIds; // size=3
	UPROPERTY() FName CurrentRestockKey = NAME_None;

	UCombatCharacterRegistrySubsystem* GetRegistry() const;

	void PushPartyToBond();
	void PushPartyLevelToShop();
};