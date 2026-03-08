#pragma once

#include"CoreMinimal.h"
#include"Engine/DataAsset.h"
#include"GameplayTagContainer.h"
#include"GroggyTypes.generated.h"

UENUM(BlueprintType)
enum class EGroggyPhase :uint8
{
  Normal  UMETA(DisplayName="Normal"),
  Stunned UMETA(DisplayName="Stunned"),
  Rising  UMETA(DisplayName="Rising"),
};

USTRUCT(BlueprintType)
struct FGroggySettings
{
  GENERATED_BODY()

  // 문서: BreakMax, DecayPerSec, StunDurationSec, RisingDurationSec, RisingBreakGainMultiplier, LockWhileStunned, ImmuneTags :contentReference[oaicite:8]{index=8}
  UPROPERTY(EditAnywhere) float BreakMax = 100.f;
  UPROPERTY(EditAnywhere) float DecayPerSec = 0.f;

  UPROPERTY(EditAnywhere) float StunDurationSec = 6.f;
  UPROPERTY(EditAnywhere) float RisingDurationSec = 2.f;

  // 문서 기본값: Rising 동안 BreakGainMultiplier=0(면역) :contentReference[oaicite:9]{index=9}
  UPROPERTY(EditAnywhere) float RisingBreakGainMultiplier = 0.f;// 0 or 0.25 등

  // 문서 : 스턴 중 게이지 고정(true) :contentReference[oaicite:10]{index=10}
  UPROPERTY(EditAnywhere) bool bLockWhileStunned = true;

  // 기본 저항(없으면 0). 문서의 "BreakResistFactor"를 수용하기 위한 필드 :contentReference[oaicite:11]{index=11}
  UPROPERTY(EditAnywhere) float BaseBreakResistFactor = 0.f;// 0~1

  // 보스/특수 면역 태그 (예: Boss.ImmuneBreak 같은 추가 태그) :contentReference[oaicite:12]{index=12}
  UPROPERTY(EditAnywhere) FGameplayTagContainer ExtraImmuneTags;
};

USTRUCT(BlueprintType)
struct FGroggySnapshot
{
  GENERATED_BODY()

  UPROPERTY(VisibleAnywhere) EGroggyPhase Phase = EGroggyPhase::Normal;
  UPROPERTY(VisibleAnywhere) float BreakValue = 0.f;
  UPROPERTY(VisibleAnywhere) float BreakMax = 100.f;
  UPROPERTY(VisibleAnywhere) float BreakRatio = 0.f;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGroggyPhaseChanged, AActor*/*Target*/, EGroggyPhase/*From*/, EGroggyPhase/*To*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnBreakValueChanged, AActor*/*Target*/, float/*OldValue*/, float/*NewValue*/, float/*BreakMax*/);

/**
 * Status 시스템(상태이상 시스템.pdf)의 "단일 진입점 ApplyStatus" 및 "태그/면역/저항" 구조와 맞물리기 위한 최소 인터페이스.
 * - 스턴/라이징은 Status로 관리 (문서) :contentReference[oaicite:13]{index=13}
 */
UINTERFACE()
class UCombatStatusAccess :public UInterface
{
  GENERATED_BODY()
};

class ICombatStatusAccess
{
GENERATED_BODY()

public:
// 태그/면역 체크 (예: Immune.Break) :contentReference[oaicite:14]{index=14}
virtual bool HasTag(const FGameplayTag& Tag) const = 0;

// 디버프의 "증폭/저항" 값을 합산해서 제공 (BreakVulnFactor, BreakResistFactor) :contentReference[oaicite:15]{index=15}
virtual float GetTotalMagnitudeByTag(const FGameplayTag& Tag) const = 0;

// Status 적용(예: CC.Stun, State.Rising, Debuff.GroggyVulnerable 등)
virtual bool ApplyStatusById(const FName& StatusId, float DurationSec, float Magnitude, int32 Stacks, const FGameplayTagContainer& Tags) = 0;

// Status 제거/만료 이벤트 구독: OnStatusRemoved(Reason=Expired/Dispel/Death/SessionEnd) :contentReference[oaicite:16]{index=16}
virtual FSimpleMulticastDelegate& OnAnyStatusChanged() = 0;// 단순화(프로젝트 StatusComponent에 맞게 교체 가능)
};