// Source/JRPGCombat/Private/Combat/Progression/Bond/BondWalkComponent.cpp
#include "Combat/Progression/Bond/BondWalkComponent.h"
#include "Combat/Progression/Bond/BondSubsystem.h"
#include "Combat/Progression/Bond/BondSettingsDataAsset.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UBondWalkComponent::UBondWalkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBondWalkComponent::BeginPlay()
{
	Super::BeginPlay();
	LastLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(Timer, this, &UBondWalkComponent::Sample, SampleIntervalSec, true);
	}
}

void UBondWalkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(Timer);
	}
	Super::EndPlay(EndPlayReason);
}

UBondSubsystem* UBondWalkComponent::GetBond() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UBondSubsystem>()
		       : nullptr;
}

const UBondSettingsDataAsset* UBondWalkComponent::GetSettings() const
{
	if (SettingsOverride) return SettingsOverride;
	if (UBondSubsystem* B = GetBond()) return B->Settings;
	return nullptr;
}

void UBondWalkComponent::Sample()
{
	if (!GetOwner())return;
	if (UGameplayStatics::IsGamePaused(GetWorld()))return;

	const UBondSettingsDataAsset* S = GetSettings();
	UBondSubsystem* Bond = GetBond();
	if (!S || !Bond)return;

	const FVector Cur = GetOwner()->GetActorLocation();
	const float Dist = FVector::Distance(Cur, LastLoc);

	const float Speed = (SampleIntervalSec > 0.f) ? (Dist / SampleIntervalSec) : 0.f;
	if (Speed < MinMoveSpeedCmPerSec)
	{
		// “휴식/메뉴 장시간 정지 시 타이머 정지”에 가까운 처리 :contentReference[oaicite:57]{index=57}
		LastLoc = Cur;
		return;
	}

	AccumTimeSec += SampleIntervalSec;
	AccumDistanceCm += Dist;

	const bool bTimeReady = (S->WalkTickSec > 0.f) && (AccumTimeSec >= S->WalkTickSec);
	const bool bDistReady = (S->WalkDistanceCm > 0.f) && (AccumDistanceCm >= S->WalkDistanceCm);

	if (bTimeReady || bDistReady)
	{
		// 현재 파티 3인 기준으로 페어 3개에 지급(SSOT: (A-B),(B-C),(A-C)) :contentReference[oaicite:58]{index=58}
		// 파티는 전투 캐릭터 시스템이 BondSubsystem.SetCurrentParty로 항상 최신 유지하는 전제
		const TArray<FName> Party = Bond->GetTrioBondLevelForCurrentParty() > 0 ? TArray<FName>() : TArray<FName>();

		// 파티는 BondSubsystem 내부에 있으므로, 여기선 “3인 파티가 세팅되어 있으면”만 지급
		// (안전: 세팅 전에는 지급하지 않음)
		// 현재 파티는 Save/Subsystem에서 관리하므로, AddBondPoints를 “Trio 참가자”로 호출하면
		// 내부에서 분배 정책에 따라 페어로 분배됨.
		//FBondAddRequest R;
		//R.Source = EBondSource::Walk;
		//R.Participants = Bond->GetExpBonusMultiplierForCurrentParty() >= 0.f
		//	                 ? Bond->GetBondState_Trio(FBondTrioId()).BondLevel, TArray<FName>()
		//	                 : TArray<FName>(); // dummy to silence warnings
		// ↑ 위 줄은 컴파일 용이성을 위해 남기면 안 됨. 아래 “실제 구현”을 사용해줘.

		// ---- 실제 구현(정상) ----
		// BondSubsystem이 CurrentPartyIds를 갖고 있으므로, 여기서는
		// “현재 파티를 외부에서 직접 받는 함수” 대신,
		// ‘Trio 이벤트로 지급’ API를 사용(Subsystem이 파티를 이미 알고 있다는 전제).
		// 가장 안전한 패턴은 전투 캐릭터 시스템에서 주기적으로(또는 파티 변경 시)
		// BondSubsystem.SetCurrentParty({A,B,C})를 호출하는 것.

		// => 따라서 WalkComponent는 현재 파티를 별도로 받지 않고,
		// 전투 캐릭터 시스템이 SetCurrentParty를 완료한 상태에서만 동작해야 한다.
		// 아래는 파티가 세팅됐을 때만 지급하는 최소 구현:
		// (파티 확인을 위해, BondSubsystem.GetExpBonusMultiplierForCurrentParty() 호출은 항상 가능)

		// 지급: “Trio 참가자”로 호출 → Subsystem이 설정에 따라 페어 분배까지 수행
		FBondAddRequest Req;
		Req.Source = EBondSource::Walk;

		// 파티 3인 정보는 BondSubsystem이 내부 저장하지만, 이 컴포넌트도 알아야 하므로
		// 실제 프로젝트에서는 PartyManager/CombatCharacterSystem에서
		// WalkComponent에 PartyIds를 주입하거나,
		// WalkComponent를 아예 PartyManager 쪽으로 옮기는 걸 권장.
		//
		// 여기선 “Owner가 PlayerCharacter이고, 그 캐릭터에 PartyIds를 제공하는 인터페이스가 있다”는 가정 없이,
		// 안전하게: 파티가 세팅되지 않았으면 skip.
		//
		// 최소 완전 동작을 위해, 아래 두 줄을 사용하려면
		// BondSubsystem.h에 `GetCurrentPartyIds()` getter를 추가해야 한다.
		//
		const TArray<FName>& PartyIds = Bond->GetCurrentPartyIds();
		Req.Participants = PartyIds;

		// --- getter 없이도 동작하게 하려면: 전투 캐릭터 시스템에서 Walk BP를 직접 AddBondPoints로 쏘는 방식을 추천 ---
		// 이 컴포넌트는 “샘플 구현”으로 남기고, 실사용은 CombatCharacterSystem에서 처리해도 됨.

		// 여기서는 컴파일/동작 완결을 위해: PartyIds getter를 BondSubsystem에 포함해두었다고 가정하고 아래를 사용:
		//const TArray<FName>& PartyIds = *(const TArray<FName>*)((const void*)&Bond); // 금지(해킹) -> 실제 사용 금지

		// 위 한 줄은 절대 쓰면 안 됨. 아래 “정상 구현”을 위해 BondSubsystem에 getter를 추가해줘:
		// (아래는 주석으로 남김)

		if (PartyIds.Num() == 3)
		{
			Req.Participants = PartyIds;
			Req.BaseAmount = S->WalkBPBase;
			Req.Context = "WalkTick";
			Req.SourceTag = "Bond.BP.Gained";
			Req.WorldLocation = Cur;
			Bond->AddBondPoints(Req);
		}


		// 누산 리셋
		AccumTimeSec = 0.f;
		AccumDistanceCm = 0.f;
	}

	LastLoc = Cur;
}
