#include "Combat/Groggy/CombatGroggyComponent.h"

#include "Combat/Core/CombatTags.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogGroggy,Log,All);

UCombatGroggyComponent::UCombatGroggyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCombatGroggyComponent::BeginPlay()
{
    Super::BeginPlay();
	
    ResolveSettings();
    ResolveStatusAccess();

    LastBreakInputReal =NowReal();
    
    // Status가 있으면 SSOT는 Status 만료 이벤트로 맞추는 것을 우선으로 한다.  :contentReference[oaicite:21]{index=21}
    if (StatusAccess)
    {
        bUseInternalTimers =false;
                
        // 프로젝트 StatusComponent가 "OnStatusRemoved"를 제공하면 그걸로 교체.
        // 여기서는 단순화된 OnAnyStatusChanged()를 구독해서 Stun/Rising 유지 여부를 폴링한다.
        StatusAccess->OnAnyStatusChanged().AddLambda([this]()
        {
            // Stun/Rising 상태가 외부에서 제거(정화/강제해제)될 수 있으므로 재동기화
            // - 실제 프로젝트에서는 "Removed된 StatusId"를 전달하는 델리게이트로 정확히 처리하는 것을 권장.
            const bool bHasStunTag =StatusAccess->HasTag(FCombatTags::CC_Stun());
            const bool bHasRisingTag =StatusAccess->HasTag(FCombatTags::State_Rising());

            if (Phase== EGroggyPhase::Stunned&&!bHasStunTag)
            {
                EnterRising_Internal(FCombatTags::Reason_StunEnded());
            }
            else if (Phase== EGroggyPhase::Rising&&!bHasRisingTag)
            {
                EnterNormal_Internal(FCombatTags::Reason_RisingEnded());
            }
        });
    }
}

void UCombatGroggyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

double UCombatGroggyComponent::NowReal()const
{
    if (const UWorld* W =GetWorld())
    {
        return static_cast<double>(W->GetRealTimeSeconds());
    }
    return 0.0;
}

void UCombatGroggyComponent::ResolveSettings()
{
    CachedSettings = FallbackSettings;

    if (SettingsAsset&& EnemyTypeId != NAME_None)
    {
        FGroggySettings FromAsset;
        if (SettingsAsset->TryGetSettings(EnemyTypeId, FromAsset))
        {
            CachedSettings = FromAsset;
        }
    }

    // 안전장치
    CachedSettings.BreakMax = FMath::Max(1.f,CachedSettings.BreakMax);
    CachedSettings.DecayPerSec = FMath::Max(0.f,CachedSettings.DecayPerSec);
    CachedSettings.StunDurationSec = FMath::Max(0.f,CachedSettings.StunDurationSec);
    CachedSettings.RisingDurationSec = FMath::Max(0.f,CachedSettings.RisingDurationSec);
    CachedSettings.RisingBreakGainMultiplier = FMath::Clamp(CachedSettings.RisingBreakGainMultiplier,0.f,1.f);
    CachedSettings.BaseBreakResistFactor = FMath::Clamp(CachedSettings.BaseBreakResistFactor,0.f,1.f);
}

void UCombatGroggyComponent::ResolveStatusAccess()
{
    StatusAccess = nullptr;

    // Owner의 컴포넌트 중 ICombatStatusAccess 구현체 탐색
    if (AActor* Owner = GetOwner())
    {
        const TSet<UActorComponent*> Components = Owner->GetComponents();
        for (UActorComponent* C :Components)
        {
            if (!C) continue;
            if (C->GetClass()->ImplementsInterface(UCombatStatusAccess::StaticClass()))
            {
				StatusAccess.SetObject(C);
				StatusAccess.SetInterface(Cast<ICombatStatusAccess>(C));
				break;
            }
        }
    }
}

FGroggySnapshot UCombatGroggyComponent::GetSnapshot()const
{
	FGroggySnapshot S;
	S.Phase =Phase;
	S.BreakValue =BreakValue;
	S.BreakMax =CachedSettings.BreakMax;
	S.BreakRatio = (CachedSettings.BreakMax<=0.f) ?0.f : (BreakValue/CachedSettings.BreakMax);
	return S;
}

void UCombatGroggyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const double Now = NowReal();

	// 내부 타이머 모드일 때 Stun/Rising 전환 처리
	if (bUseInternalTimers)
	{
		if ((Phase == EGroggyPhase::Stunned || Phase == EGroggyPhase::Rising) && PhaseEndReal > 0.0 && Now >= PhaseEndReal)
		{
			if (Phase == EGroggyPhase::Stunned) EnterRising_Internal(FCombatTags::Reason_StunEnded());
			else if (Phase == EGroggyPhase::Rising) EnterNormal_Internal(FCombatTags::Reason_RisingEnded());
		}
	}

	// Break 감쇠(문서: DecayPerSec, RealTime 기준) :contentReference[oaicite:22]{index=22} :contentReference[oaicite:23]{index=23}
	ApplyDecayIfNeeded(Now, DeltaTime);
}

void UCombatGroggyComponent::ApplyDecayIfNeeded(double NowRealSeconds, float DeltaTime)
{
	if (Phase != EGroggyPhase::Normal) return;
	if (CachedSettings.DecayPerSec <= 0.f) return;
	if (BreakValue <= 0.f) return;

	// "브레이크 입력 없을 때 감쇠" (문서) :contentReference[oaicite:24]{index=24}
	const double IdleSec = NowRealSeconds - LastBreakInputReal;
	if (IdleSec < 0.25) return; // 작은 버퍼(연타 중 떨림 방지)

	const float Old = BreakValue;
	BreakValue = FMath::Max(0.f, BreakValue - CachedSettings.DecayPerSec * DeltaTime);

	if (!FMath::IsNearlyEqual(Old, BreakValue))
	{
		OnBreakValueChanged.Broadcast(GetOwner(),Old,BreakValue,CachedSettings.BreakMax);
	}
}

bool UCombatGroggyComponent::AddBreak(AActor* SourceActor, float BreakAmountRaw, const FGameplayTagContainer& ContextTags)
{
	if (!GetOwner()) return false;
	if (BreakAmountRaw <= 0.f) return false;

	// 스턴 중 게이지 잠금:contentReference[oaicite:25]{index=25}
	if (Phase == EGroggyPhase::Stunned && CachedSettings.bLockWhileStunned)
	{
		UE_LOG(LogGroggy, Verbose, TEXT("[%s] AddBreak ignored: locked while stunned"), *GetOwner()->GetName());
		return false;
	}

	// 면역: StatusTag Immune.Break 보유 시 0 처리 :contentReference[oaicite:26]{index=26}
	if (StatusAccess && StatusAccess->HasTag(FCombatTags::Immune_Break()))
	{
		UE_LOG(LogGroggy, Verbose, TEXT("[%s] AddBreak rejected: Immune.Break"), *GetOwner()->GetName());
		return false;
	}

	// 추가 면역(EnemyType/보스용)
	if (StatusAccess)
	{
		for (const FGameplayTag& T : CachedSettings.ExtraImmuneTags)
		{
			if (StatusAccess->HasTag(T))
			{
				UE_LOG(LogGroggy, Verbose, TEXT("[%s] AddBreak rejected: ExtraImmuneTag %s"), *GetOwner()->GetName(), *T.ToString());
				return false;
			}
		}
	}

	float BreakAmount = BreakAmountRaw;

	// 라이징 중 브레이크 획득 처리: 기본 0(면역) 또는 0.25 등 :contentReference[oaicite:27]{index=27} :contentReference[oaicite:28]{index=28}
	if (Phase == EGroggyPhase::Rising)
	{
		BreakAmount *= CachedSettings.RisingBreakGainMultiplier;
		if (BreakAmount <=0.f)
		{
			return false;
		}
	}

	// 저항/취약 반영  :contentReference[oaicite:29]{index=29}
	float BreakResistFactor = CachedSettings.BaseBreakResistFactor;
	float BreakVulnFactor = 0.f;

	if (StatusAccess)
	{
		// Debuff.BreakVuln의 Magnitude 총합을 "취약 계수(+X%)"로 간주 :contentReference[oaicite:30]{index=30}
		BreakVulnFactor = FMath::Max(0.f, StatusAccess->GetTotalMagnitudeByTag(FCombatTags::Debuff_BreakVuln()));

		// Debuff.BreakResistDown은 "저항 감소(-Y%)"이므로 ResistFactor에서 빼는 방식으로 반영 :contentReference[oaicite:31]{index=31}
		const float ResistDown = FMath::Max(0.f, StatusAccess->GetTotalMagnitudeByTag(FCombatTags::Debuff_BreakResistDown()));
		BreakResistFactor = FMath::Clamp(BreakResistFactor - ResistDown, 0.f, 1.f);
	}

	BreakAmount *= (1.f - BreakResistFactor);
	BreakAmount *= (1.f + BreakVulnFactor);

	if (BreakAmount <= 0.f) return false;

	// 누적: BreakValue = clamp(BreakValue + BreakAmount, 0, BreakMax) :contentReference[oaicite:32]{index=32}
	const float Old = BreakValue;
	BreakValue = FMath::Clamp(BreakValue+BreakAmount, 0.f, CachedSettings.BreakMax);

	LastBreakInputReal = NowReal();

	if (!FMath::IsNearlyEqual(Old, BreakValue))
	{
		OnBreakValueChanged.Broadcast(GetOwner(), Old, BreakValue, CachedSettings.BreakMax);
	}

	// 임계 검사: if BreakValue >= BreakMax → EnterStun() :contentReference[oaicite:33]{index=33}
	if (BreakValue >= CachedSettings.BreakMax - KINDA_SMALL_NUMBER)
	{
		EnterStun_Internal(FCombatTags::Reason_BreakReached());
	}

	// 텔레메트리(권장): Combat.Break.Added :contentReference[oaicite:34]{index=34}
	UE_LOG(LogGroggy, Verbose, TEXT("[%s] BreakAdded delta=%.2f value=%.2f/%0.2f phase=%d"),
		*GetOwner()->GetName(), BreakAmount, BreakValue, CachedSettings.BreakMax, (int32)Phase);

	return true;
}

void UCombatGroggyComponent::SetPhase(EGroggyPhase NewPhase, const FGameplayTag& ReasonTag)
{
	if (Phase == NewPhase)return;

	const EGroggyPhase From = Phase;
	Phase = NewPhase;

	// PhaseChanged 이벤트 :contentReference[oaicite:35]{index=35}
	OnGroggyPhaseChanged.Broadcast(GetOwner(), From, NewPhase);

	UE_LOG(LogGroggy,Log,TEXT("[%s] PhaseChanged %d -> %d (Reason=%s)"),
		*GetOwner()->GetName(), (int32)From, (int32)NewPhase, *ReasonTag.ToString());

	// 텔레메트리: Combat.Groggy.PhaseChanged :contentReference[oaicite:36]{index=36}
}

void UCombatGroggyComponent::EnterStun_Internal(const FGameplayTag& ReasonTag)
{
	// 문서: EnterStun() 처리 - Phase=Stunned, BreakValue=0, Stun 상태 적용, 피해증폭 상태 적용 :contentReference[oaicite:37]{index=37}
	SetPhase(EGroggyPhase::Stunned, ReasonTag);

	const float Old = BreakValue;
	BreakValue =0.f;
	if (!FMath::IsNearlyEqual(Old, BreakValue))
	{
		OnBreakValueChanged.Broadcast(GetOwner(), Old, BreakValue, CachedSettings.BreakMax);
	}

	if (StatusAccess)
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FCombatTags::CC_Stun());
		StatusAccess->ApplyStatusById(StunStatusId, CachedSettings.StunDurationSec, /*Magnitude*/1.f, /*Stacks*/1, Tags);

		// 피해 증폭은 별도 상태로 처리(선택) :contentReference[oaicite:38]{index=38}
		if (GroggyVulnerableStatusId != NAME_None)
		{
			FGameplayTagContainer VulnTags;
			VulnTags.AddTag(FCombatTags::Debuff_BreakVuln()); // 프로젝트 정책에 맞게 변경 가능
			StatusAccess->ApplyStatusById(GroggyVulnerableStatusId, CachedSettings.StunDurationSec, /*Magnitude*/1.f, /*Stacks*/1, VulnTags);
		}
	}
	else
	{
		// 백업: 내부 타이머
		bUseInternalTimers = true;
		PhaseEndReal = NowReal() + CachedSettings.StunDurationSec;
	}
}

void UCombatGroggyComponent::EnterRising_Internal(const FGameplayTag& ReasonTag)
{
	// 문서: Stun 만료 후 Rising 진입, Rising은 State.Rising 태그로 제공 가능 :contentReference[oaicite:39]{index=39}
	SetPhase(EGroggyPhase::Rising, ReasonTag);

	if (StatusAccess)
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FCombatTags::State_Rising());
		StatusAccess->ApplyStatusById(RisingStatusId, CachedSettings.RisingDurationSec, /*Magnitude*/1.f, /*Stacks*/1,Tags);
	}
	else
	{
		bUseInternalTimers = true;
		PhaseEndReal = NowReal()+CachedSettings.RisingDurationSec;
	}
}

void UCombatGroggyComponent::EnterNormal_Internal(const FGameplayTag& ReasonTag)
{
	SetPhase(EGroggyPhase::Normal, ReasonTag);

	// 문서: Rising 종료 후 Normal 복귀, Break는 0에서 다시 시작 :contentReference[oaicite:40]{index=40}
	const float Old = BreakValue;
	BreakValue = 0.f;
	if (!FMath::IsNearlyEqual(Old, BreakValue))
	{
		OnBreakValueChanged.Broadcast(GetOwner(), Old, BreakValue, CachedSettings.BreakMax);
	}

	PhaseEndReal = 0.0;
}

void UCombatGroggyComponent::ForceEnterStun()
{
	EnterStun_Internal(FCombatTags::Reason_BreakReached());
}

void UCombatGroggyComponent::ForceExitToNormal()
{
	EnterNormal_Internal(FCombatTags::Reason_RisingEnded());
}