#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Tactical/TacticalTypes.h"
#include "TacticalSettingsDataAsset.generated.h"

USTRUCT()
struct FTacticalSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) float TacticalSlowScale = 0.15f; // 0.10~0.20 권장 :content Reference[oaicite:9]{index=9}
	UPROPERTY(EditDefaultsOnly) float TacticalMaxDurationRealSec = 5.0f; // ≤ 5.0 :content Reference[oaicite:10]{index=10}
	UPROPERTY(EditDefaultsOnly) float BlendInSec = 0.10f;
	UPROPERTY(EditDefaultsOnly) float BlendOutSec = 0.12f;

	UPROPERTY(EditDefaultsOnly) bool bAllowReserveWhenUnavailable = true; // 예약 시 CanUse 강제 X :contentReference[oaicite:11]{index=11}
	UPROPERTY(EditDefaultsOnly) bool bToggleSameSkillClears = true; // 같은 스킬 다시 클릭 -> 해제 :contentReference[oaicite:12]{index=12}

	// 문서: 전술 종료 시 예약 유지가 기본 : content Reference [oaicite:13]{index=13}
	UPROPERTY(EditDefaultsOnly) bool bKeepReservationsOnExit =true;

	// 세션 강제 종료(Ending/Cleanup) 때 예약을 유지/정리 중 하나를 고정해야 함 :content Reference [oaicite:14]{index=14}
	UPROPERTY(EditDefaultsOnly) bool bClearReservationsOnForcedExit =false;
};

UCLASS()
class JRPGCOMBAT_API UTacticalSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly) FTacticalSettings Settings;
};