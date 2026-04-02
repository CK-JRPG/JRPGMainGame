#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CombatViewModels.generated.h"

/**
 *  1. ÆÄÆ¼ ½½·Ô ºä¸ðµ¨ (HP, AP Ã³¸®)
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNameUpdated, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHPUIUpdated, float /*Percent*/, const FString& /*Text*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAPUIUpdated, float /*Percent*/);

UCLASS()
class JRPG_API UCombatPartySlotViewModel : public UObject
{
	GENERATED_BODY()
public:
	void BindToActor(AActor* MemberActor);
	void Unbind();

	FOnNameUpdated OnNameUpdated;
	FOnHPUIUpdated OnHPUIUpdated;
	FOnAPUIUpdated OnAPUIUpdated;

private:
	TWeakObjectPtr<class UHPComponent> CachedHPComp;
	TWeakObjectPtr<class UAPComponent> CachedAPComp;

	void HandleHPChanged(float OldHP, float NewHP, FName Reason);
	void HandleAPChanged(int32 OldAP, int32 NewAP, FName Reason);
};

/// <summary>
/// Àû/Å¸°Ù ºä¸ðµ¨ (HP, Groggy Ã³¸®)
/// </summary>
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTargetGroggyUpdated, bool /*bGroggy*/);

UCLASS()
class JRPG_API UEnemyViewModel : public UObject
{
	GENERATED_BODY()
public:
	void BindToEnemy(AActor* EnemyActor);
	void Unbind();

	FOnNameUpdated OnTargetNameUpdated;
	FOnHPUIUpdated OnTargetHPUpdated;
	FOnTargetGroggyUpdated OnTargetGroggyUpdated;

private:
	TWeakObjectPtr<class UHPComponent> CachedHPComp;
	TWeakObjectPtr<class UGroggyComponent> CachedGroggyComp;

	void HandleHPChanged(float OldHP, float NewHP, FName Reason);
	void HandleGroggyCHanged(bool bGroggy);
};

/// <summary>
/// ¾×¼Ç ÆÈ·¹Æ® ºä¸ðµ¨ (SP Ã³¸®)
/// </summary>
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSPUIUpdated, float /*Percent*/, const FString& /*Text*/);

UCLASS()
class JRPG_API UActionPalettedViewModel : public UObject
{
	GENERATED_BODY()
public:
	void BindToPlayer(AActor* PlayerActor);
	void Unbind();
	FOnSPUIUpdated OnSPUIUpdated;

private:
	TWeakObjectPtr<class USPComponent> CachedSPComp;
	void HandleSPChanged(int32 OldSP, int32 NewSP, FName Reason);
};
