#include "UI/ViewModels/CombatViewModels.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "GameFramework/Actor.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"
#include "Engine/AssetManager.h"

// --- Party Slot ViewModel ---
// 실시간 전투에서 사용
void UCombatPartySlotViewModel::BindToActor(AActor* MemberActor) {

    if (!MemberActor) return;
    //OnNameUpdated.Broadcast(MemberActor->GetName());

    if (UHPComponent* HPComp = MemberActor->FindComponentByClass<UHPComponent>()) {
        CachedHPComp = HPComp;
        HPComp->OnHPChanged.AddUObject(this, &UCombatPartySlotViewModel::HandleActorHPChanged);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatViewModels::BindToActor: %s이 HPComp를 소유하지 않음"), *MemberActor->GetName());
    }
    if (UAPComponent* APComp = MemberActor->FindComponentByClass<UAPComponent>()) {
        CachedAPComp = APComp;
        APComp->OnAPChanged.AddUObject(this, &UCombatPartySlotViewModel::HandleActorAPChanged);
        HandleActorAPChanged(APComp->GetAP(), APComp->GetAP(), NAME_None);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatViewModels::BindToActor: %s이 APComp를 소유하지 않음"), *MemberActor->GetName());
    }

}

// 서브 시스템에서 불러와 사용
void UCombatPartySlotViewModel::BindToCharacter(FName InCharacterID)
{
    Unbind();
    BoundCharacterID = InCharacterID;

    if (BoundCharacterID.IsNone() || !GetWorld() || !GetWorld()->GetGameInstance()) return;

    if (UCharacterRuntimeSubsystem* RuntimeSub = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterRuntimeSubsystem>())
    {
        OnNameUpdated.Broadcast(BoundCharacterID.ToString());

        if (const FCharacterResourceSnapshot* Snap = RuntimeSub->GetSnapshot(BoundCharacterID))
        {
            HandleSubsystemHPChanged(BoundCharacterID, Snap->HP, Snap->MaxHP);
            HandleSubsystemAPChanged(BoundCharacterID, Snap->AP, Snap->MaxAP);
        }

        RuntimeSub->OnHPChanged.AddUObject(this, &UCombatPartySlotViewModel::HandleSubsystemHPChanged);
        RuntimeSub->OnAPChanged.AddUObject(this, &UCombatPartySlotViewModel::HandleSubsystemAPChanged);
    }
}

void UCombatPartySlotViewModel::Unbind() 
{
    if (GetWorld() && GetWorld()->GetGameInstance())
    {
        if (UCharacterRuntimeSubsystem* RuntimeSub = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterRuntimeSubsystem>())
        {
            RuntimeSub->OnHPChanged.RemoveAll(this);
            RuntimeSub->OnAPChanged.RemoveAll(this);
        }
    }

    if (CachedHPComp.IsValid()) CachedHPComp->OnHPChanged.RemoveAll(this);
    if (CachedAPComp.IsValid()) CachedAPComp->OnAPChanged.RemoveAll(this);

    BoundCharacterID = NAME_None;
}

void UCombatPartySlotViewModel::Refresh()
{
    if (!BoundCharacterID.IsNone())
    {
        FPrimaryAssetId AssetId = FPrimaryAssetId(FName("CombatCharacterData"), BoundCharacterID);

        if (UAssetManager* AssetMgr = UAssetManager::GetIfValid())
        {
            UObject* LoadedAsset = AssetMgr->GetPrimaryAssetObject(AssetId);

            if (!LoadedAsset)
            {
                FSoftObjectPath AssetPath = AssetMgr->GetPrimaryAssetPath(AssetId);
                if (AssetPath.IsValid())
                {
                    LoadedAsset = AssetPath.TryLoad();
                }
            }

            if (const UCombatCharacterDataAsset* DA = Cast<UCombatCharacterDataAsset>(LoadedAsset))
            {
                OnNameUpdated.Broadcast(DA->DisplayName.ToString());
            }
            else
            {
                OnNameUpdated.Broadcast(BoundCharacterID.ToString());
                UE_LOG(LogTemp, Warning, TEXT("에셋 매니저: ID : '%s' 데이터를 찾지 못했습니다!폴더 경로와 CharacterId를 확인하세요."), *BoundCharacterID.ToString());
            }
        }
    }
    if (CachedHPComp.IsValid()) {
        HandleActorHPChanged(0.f, CachedHPComp->GetHP(), NAME_None);
    }
    else if (!BoundCharacterID.IsNone()) {
        if (UCharacterRuntimeSubsystem* RuntimeSub = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterRuntimeSubsystem>()) {
            if (const FCharacterResourceSnapshot* Snap = RuntimeSub->GetSnapshot(BoundCharacterID)) {
                HandleSubsystemHPChanged(BoundCharacterID, Snap->HP, Snap->MaxHP);
            }
        }
    }

    if (CachedAPComp.IsValid()) {
        HandleActorAPChanged(0, CachedAPComp->GetAP(), NAME_None);
    }
    else if (!BoundCharacterID.IsNone()) {
        if (UCharacterRuntimeSubsystem* RuntimeSub = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterRuntimeSubsystem>()) {
            if (const FCharacterResourceSnapshot* Snap = RuntimeSub->GetSnapshot(BoundCharacterID)) {
                HandleSubsystemAPChanged(BoundCharacterID, Snap->AP, Snap->MaxAP);
            }
        }
    }
}

// Exploration UI 전용 함수
void UCombatPartySlotViewModel::HandleSubsystemHPChanged(FName CharID, float NewHP, float MaxHP)
{
    if (CharID != BoundCharacterID) return;

    float Percent = MaxHP > 0.f ? NewHP / MaxHP : 0.f;
    FString Text = FString::Printf(TEXT("%d"), FMath::RoundToInt(NewHP));
    OnHPUIUpdated.Broadcast(Percent, Text);
}

void UCombatPartySlotViewModel::HandleSubsystemAPChanged(FName CharID, int32 NewAP, int32 MaxAP)
{
    if (CharID != BoundCharacterID) return;

    float Percent = MaxAP > 0.f ? (float)NewAP / MaxAP : 0.f;
    OnAPUIUpdated.Broadcast(Percent);
}

// Combat UI 전용 함수
void UCombatPartySlotViewModel::HandleActorHPChanged(float OldHP, float NewHP, FName Reason)
{
    if (CachedHPComp.IsValid()) {
        float MaxHP = CachedHPComp->MaxHP;
        float Percent = MaxHP > 0.f ? NewHP / MaxHP : 0.f;
        FString Text = FString::Printf(TEXT("%d"), FMath::RoundToInt(NewHP));
        OnHPUIUpdated.Broadcast(Percent, Text);
    }
}
void UCombatPartySlotViewModel::HandleActorAPChanged(int32 OldAP, int32 NewAP, FName Reason)
{
    if (CachedAPComp.IsValid()) {
        float MaxAP = CachedAPComp->MaxAP;
        float Percent = MaxAP > 0.f ? (float)NewAP / MaxAP : 0.f;
        OnAPUIUpdated.Broadcast(Percent);
    }
}

// --- Enemy ViewModel ---
void UEnemyViewModel::BindToEnemy(AActor* EnemyActor) {
    Unbind();
    if (!EnemyActor) return;
    OnTargetNameUpdated.Broadcast(EnemyActor->GetName());

    if (UHPComponent* HPComp = EnemyActor->FindComponentByClass<UHPComponent>()) {
        CachedHPComp = HPComp;
        HPComp->OnHPChanged.AddUObject(this, &UEnemyViewModel::HandleActorHPChanged);
        HandleActorHPChanged(HPComp->GetHP(), HPComp->GetHP(), NAME_None);
    }
    if (UGroggyComponent* GroggyComp = EnemyActor->FindComponentByClass<UGroggyComponent>()) {
        CachedGroggyComp = GroggyComp;
        GroggyComp->OnGroggyStateChanged.AddUObject(this, &UEnemyViewModel::HandleGroggyChanged);
        HandleGroggyChanged(false); // 초기값
    }
}
void UEnemyViewModel::Unbind() {
    if (CachedHPComp.IsValid()) CachedHPComp->OnHPChanged.RemoveAll(this);
    if (CachedGroggyComp.IsValid()) CachedGroggyComp->OnGroggyStateChanged.RemoveAll(this);
}
void UEnemyViewModel::HandleActorHPChanged(float OldHP, float NewHP, FName Reason) {
    if (CachedHPComp.IsValid()) {
        float MaxHP = CachedHPComp->MaxHP;
        float Percent = MaxHP > 0.f ? NewHP / MaxHP : 0.f;
        FString Text = FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(NewHP), FMath::RoundToInt(MaxHP));
        OnTargetHPUpdated.Broadcast(Percent, Text);
    }
}
void UEnemyViewModel::HandleGroggyChanged(bool bGroggy) {
    OnTargetGroggyUpdated.Broadcast(bGroggy);
}

// --- Action Palette ViewModel ---
void UActionPaletteViewModel::BindToPlayer(AActor* PlayerActor) {
    Unbind();
    if (!PlayerActor) return;
    if (USPComponent* SPComp = PlayerActor->FindComponentByClass<USPComponent>()) {
        CachedSPComp = SPComp;
        SPComp->OnSPChanged.AddUObject(this, &UActionPaletteViewModel::HandleSPChanged);
        HandleSPChanged(SPComp->GetSP(), SPComp->GetSP(), NAME_None);
    }
}
void UActionPaletteViewModel::Unbind() {
    if (CachedSPComp.IsValid()) CachedSPComp->OnSPChanged.RemoveAll(this);
}
void UActionPaletteViewModel::HandleSPChanged(int32 OldSP, int32 NewSP, FName Reason) {
    if (CachedSPComp.IsValid()) {
        int32 MaxSP = CachedSPComp->MaxSP;
        float Percent = MaxSP > 0 ? (float)NewSP / MaxSP : 0.f;
        FString Text = FString::Printf(TEXT("%d / %d"), NewSP, MaxSP);
        OnSPUIUpdated.Broadcast(Percent, Text);
    }
}