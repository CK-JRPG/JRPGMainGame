#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CombatCharacterRegistrySubsystem.generated.h"

UCLASS()
class JRPG_API UCombatCharacterRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	void RegisterCharacter(FName CharacterId, AActor *Actor);
	void UnregisterCharacter(FName CharacterId, AActor *Actor);

	AActor* FindById(FName CharacterId) const;
	void GetAllCharacters(TArray<AActor*> &Out) const;

private:
	UPROPERTY() TMap<FName,TWeakObjectPtr<AActor>> Map;
};