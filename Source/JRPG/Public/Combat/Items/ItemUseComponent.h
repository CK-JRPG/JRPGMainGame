// Source/JRPGCombat/Public/Combat/Items/ItemUseComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "Combat/Core/CombatSessionProvider.h"
#include "Combat/Core/PartyProvider.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/ItemUseTypes.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/CombatLevelProvider.h"

#include "ItemUseComponent.generated.h"

class UItemDatabaseAsset;
class UItemDataAsset;

class UHPComponent;
class UAPComponent;

DECLARE_MULTICAST_DELEGATE_FiveParams(FOnItemConsumed, AActor* /*User*/, AActor* /*Target*/, FName /*ItemId*/, FGuid /*InstanceId*/, FName /*ReasonTag*/);

// 상태 시스템 붙일 연결점
UINTERFACE(MinimalAPI)
class UCombatStatusMutator : public UInterface { GENERATED_BODY() };

class ICombatStatusMutator
{
	GENERATED_BODY()
public:
	virtual bool ApplyStatusById(const FName &StatusId, float DurationSec, float Magnitude, int32 Stacks,const FGameplayTagContainer& Tags) = 0;
	virtual bool RemoveStatusById(const FName &StatusId,FName ReasonTag) = 0;
	virtual bool RemoveByTag(const FGameplayTag &Tag,FName ReasonTag) = 0;
};

// SP 시스템 붙일 연결점
UINTERFACE(MinimalAPI)
class UCombatSPMutator : public UInterface { GENERATED_BODY() };

class ICombatSPMutator
{
	GENERATED_BODY()
public:
	virtual bool GrantSP(int32 Amount, FName ReasonTag,AActor *Instigator) = 0;
};

UCLASS(ClassGroup=(Combat),meta=(BlueprintSpawnableComponent))
class JRPG_API UItemUseComponent :public UActorComponent
{
	GENERATED_BODY()

public:
	UItemUseComponent();

	UPROPERTY(EditAnywhere) TObjectPtr<UItemDatabaseAsset> ItemDB = nullptr;
	UPROPERTY(EditAnywhere) TScriptInterface<IJRPGCombatLevelProvider> LevelProvider;

	// 전투/체인/파티 제공자(없으면 제한적으로만 동작)
	UPROPERTY(EditAnywhere) TScriptInterface<ICombatSessionProvider> SessionProvider;
	UPROPERTY(EditAnywhere) TScriptInterface<IPartyProvider> PartyProvider;

	FOnItemConsumed OnItemConsumed;

	FItemOp TryUseItem(UInventorySubsystem* Inv, FGuid InstanceId, AActor* Target);

private:
	const UItemDataAsset* FindDef(FName ItemId) const;

	bool PassSessionRule(const UItemDataAsset &Def, FName &OutReason)const;
	bool PassLevelRule(const UItemDataAsset &Def, AActor *User,FName &OutReason)const;
	bool PassTargetingRule(const UItemDataAsset &Def,AActor*User,AActor* Target,FName &OutReason)const;

	FItemOp ApplyConsumableEffects(const UItemDataAsset &Def, AActor *User,AActor *Target,const FItemInstance& UsedInstance);

	void ApplyHealHP(AActor* User, AActor* Target, float Amount, FName SourceTag);
	void ApplyRestoreAP(AActor* Target, float Amount, FName SourceTag);
	void ApplyCleanseStatusId(AActor* Target,FName StatusId);
	void ApplyCleanseByTag(AActor* Target, const FGameplayTag&Tag);
	void ApplyApplyStatusId(AActor* Target,const FConsumableEffect&E);
	void ApplyGrantSP(AActor* User,int32 Amount);
};