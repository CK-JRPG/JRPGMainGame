#include "UI/Combat/CombatTagSwapSlotWidget.h"

#include "UI/ViewModels/CombatViewModels.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"

void UCombatTagSwapSlotWidget::BindSwapData(UCombatPartySlotViewModel* InVM, const FString& KeyString)
{
	if (Text_KeyBadge)
	{
		Text_KeyBadge->SetText(FText::FromString(KeyString));
	}

	if (!InVM || !Img_Portrait) return;
	FName CharID = InVM->GetCharacterID();

	// 데이터 에셋/테이블에서 CharID로 초상화 연동
    FPrimaryAssetId AssetId = FPrimaryAssetId(FName("CombatCharacterData"), CharID);

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
			Img_Portrait->SetBrushFromTexture(DA->Portrait);
        }
    }

}
