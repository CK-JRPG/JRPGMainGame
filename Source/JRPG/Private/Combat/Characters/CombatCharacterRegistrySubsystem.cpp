#include "Combat/Characters/CombatCharacterRegistrySubsystem.h"

void UCombatCharacterRegistrySubsystem::RegisterCharacter(FName CharacterId, AActor* Actor)
{
	if (CharacterId.IsNone() || !Actor) return;
	Map.Add(CharacterId, Actor);
}

void UCombatCharacterRegistrySubsystem::UnregisterCharacter(FName CharacterId, AActor* Actor)
{
	if (CharacterId.IsNone() || !Actor) return;

	const TWeakObjectPtr<AActor>* Found = Map.Find(CharacterId);
	if (!Found) return;

	if (Found->Get() == Actor)
	{
		Map.Remove(CharacterId);
	}
}

AActor* UCombatCharacterRegistrySubsystem::FindById(FName CharacterId) const
{
	if (const TWeakObjectPtr<AActor>* Found = Map.Find(CharacterId))
		return Found->Get();
	return nullptr;
}

void UCombatCharacterRegistrySubsystem::GetAllCharacters(TArray<AActor*>& Out) const
{
	Out.Reset();
	for (const auto& KV :Map)
	{
		if (AActor* A = KV.Value.Get())
			Out.Add(A);
	}
}