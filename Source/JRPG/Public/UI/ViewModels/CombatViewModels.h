#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatViewModels.generated.h"

// ---------------------------------------------------------
// 1. 파티 슬롯 뷰모델 (HP, AP 처리)
// ---------------------------------------------------------
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNameUpdated, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHPUIUpdated, float /*Percent*/, const FString& /*Text*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAPUIUpdated, float /*Percent*/);

UCLASS()
class JRPG_API UCombatPartySlotViewModel : public UObject
{
    GENERATED_BODY()
public:
    void BindToActor(AActor* MemberActor);
    void BindToCharacter(FName InCharacterID);
    void Unbind();

    FOnNameUpdated OnNameUpdated;
    FOnHPUIUpdated OnHPUIUpdated;
    FOnAPUIUpdated OnAPUIUpdated;

    FName GetCharacterID() const { return BoundCharacterID; }

private:
    FName BoundCharacterID;

    void HandleHPChanged(FName CharID, float NewHP, float MaxHP);
    void HandleAPChanged(FName CharID, int32 NewAP, int32 MaxAP);

    TWeakObjectPtr<class UHPComponent> CachedHPComp;
    TWeakObjectPtr<class UAPComponent> CachedAPComp;

    void HandleHPChanged(float OldHP, float NewHP, FName Reason);
    void HandleAPChanged(int32 OldAP, int32 NewAP, FName Reason);
};

// ---------------------------------------------------------
// 2. 적/타겟 뷰모델 (HP, Groggy 처리)
// ---------------------------------------------------------
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
    void HandleGroggyChanged(bool bGroggy);
};

// ---------------------------------------------------------
// 3. 액션 팔레트 뷰모델 (HP, AP 처리)
// ---------------------------------------------------------
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSPUIUpdated, float /*Percent*/, const FString& /*Text*/);

UCLASS()
class JRPG_API UActionPaletteViewModel : public UObject
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