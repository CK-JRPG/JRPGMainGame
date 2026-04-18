// Source/JRPGCombat/Private/Combat/Progression/Leveling/LevelingSubsystem.cpp
#include "Combat/Progression/Leveling/LevelingSubsystem.h"
#include "Combat/Progression/Leveling/LevelingSaveGameSubsystem.h"

ULevelingSaveGameSubsystem* ULevelingSubsystem::GetSaveSys() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<ULevelingSaveGameSubsystem>() : nullptr;
}

int32 ULevelingSubsystem::LevelMin() const
{
	return ExpSettings ? ExpSettings->LevelMin : 1;
}

int32 ULevelingSubsystem::LevelMax() const
{
	// 문서: LevelMax TBD (데이터로) :contentReference[oaicite:27]{index=27}
	return ExpSettings ? ExpSettings->LevelMax : 30;
}

int32 ULevelingSubsystem::ExpToNextFor(int32 Level) const
{
	if (ExpCurve) return ExpCurve->GetExpToNext(Level);
	// fallback
	return 100 + (Level * 50);
}

int32 ULevelingSubsystem::GetExpToNext() const
{
	if (PartyLevel >= LevelMax()) return 0;
	return ExpToNextFor(PartyLevel);
}

void ULevelingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadFromSave();
	QueryBondMultiplierAndBroadcastIfChanged();
}

void ULevelingSubsystem::LoadFromSave()
{
	if (ULevelingSaveGameSubsystem* SaveSys = GetSaveSys())
	{
		SaveSys->LoadOrCreate();
		if (ULevelingSaveGame* S = SaveSys->GetSave())
		{
			PartyLevel = FMath::Max(LevelMin(), S->PartyLevel);
			PartyLevel = FMath::Min(PartyLevel, LevelMax());

			CurrentExp = FMath::Max(0, S->CurrentExp);

			DiscoveredAreas = S->DiscoveredAreas;
			DiscoveredRestPoints = S->DiscoveredRestPoints;
			ClaimedExploreRewards = S->ClaimedExploreRewards;

			TravelAcc = S->TravelAcc;
		}
	}
}

void ULevelingSubsystem::FlushToSave()
{
	if (ULevelingSaveGameSubsystem* SaveSys = GetSaveSys())
	{
		if (ULevelingSaveGame* S = SaveSys->GetSave())
		{
			S->PartyLevel = PartyLevel;
			S->CurrentExp = CurrentExp;

			S->DiscoveredAreas = DiscoveredAreas;
			S->DiscoveredRestPoints = DiscoveredRestPoints;
			S->ClaimedExploreRewards = ClaimedExploreRewards;

			S->TravelAcc = TravelAcc;

			SaveSys->MarkDirty();
		}
	}
}

float ULevelingSubsystem::QueryBondMultiplierAndBroadcastIfChanged()
{
	const float NewMul = BondBonusProvider ? FMath::Max(0.0f, BondBonusProvider->GetBondExpBonusMultiplier()) : 1.0f;
	if (!FMath::IsNearlyEqual(NewMul, CachedBondMultiplier, 0.0001f))
	{
		CachedBondMultiplier = NewMul;
		OnExpBonusMultiplierChanged.Broadcast(CachedBondMultiplier); // SSOT 이벤트 :contentReference[oaicite:28]{index=28}
	}
	return CachedBondMultiplier;
}

float ULevelingSubsystem::ComputeInactivityMultiplier(double NowReal) const
{
	if (!ExpSettings) return 1.0f;

	const float Window = FMath::Max(0.f, ExpSettings->InactivityWindowSec);
	if (Window <= 0.f) return 1.0f;

	const double Dt = NowReal - LastSignificantEventReal;
	if (Dt <= (double)Window) return 1.0f;

	return FMath::Clamp(ExpSettings->InactivityMul, 0.f, 1.f);
}

void ULevelingSubsystem::NotifySignificantEvent(FName/*EventTag*/)
{
	// Travel anti-exploit “최근 N초 의미있는 사건” 조건용 :contentReference[oaicite:29]{index=29}
	LastSignificantEventReal = FPlatformTime::Seconds();
}

void ULevelingSubsystem::SetTravelAccumulator(const FTravelExpAccumulator& In)
{
	TravelAcc = In;
	FlushToSave();
}

FTravelExpAccumulator ULevelingSubsystem::GetTravelAccumulator() const
{
	return TravelAcc;
}

bool ULevelingSubsystem::IsDuplicateByContext(const FExpGrantRequest& Req, FName& OutReason) const
{
	OutReason = NAME_None;

	// 발견/상자 보상은 1회성(저장) :contentReference[oaicite:30]{index=30} :contentReference[oaicite:31]{index=31} :contentReference[oaicite:32]{index=32}
	switch (Req.Source)
	{
	case EExpSource::DiscoverArea:
		if (Req.ContextName.IsNone())
		{
			OutReason = "Reject.InvalidContext";
			return true;
		}
		if (DiscoveredAreas.Contains(Req.ContextName))
		{
			OutReason = "Reject.DuplicateDiscovery";
			return true;
		}
		return false;

	case EExpSource::DiscoverRestPoint:
		if (Req.ContextName.IsNone())
		{
			OutReason = "Reject.InvalidContext";
			return true;
		}
		if (DiscoveredRestPoints.Contains(Req.ContextName))
		{
			OutReason = "Reject.DuplicateDiscovery";
			return true;
		}
		return false;

	case EExpSource::ExploreReward:
		if (!Req.ContextGuid.IsValid())
		{
			OutReason = "Reject.InvalidContext";
			return true;
		}
		if (ClaimedExploreRewards.Contains(Req.ContextGuid))
		{
			OutReason = "Reject.DuplicateDiscovery";
			return true;
		}
		return false;

	default:
		return false;
	}
}

FExpGrantOp ULevelingSubsystem::ValidateAndApplyGrant(const FExpGrantRequest& Req, int32 BaseExpResolved)
{
	if (BaseExpResolved <= 0)
		return FExpGrantOp::Fail("Reject.InvalidContext");

	// 중복 체크(Discovery/ExploreReward) :contentReference[oaicite:33]{index=33}
	FName DupReason;
	if (IsDuplicateByContext(Req, DupReason))
		return FExpGrantOp::Fail(DupReason);

	// Bond multiplier 적용(지급 직전, 단 BondLevelUp에는 미적용) :contentReference[oaicite:34]{index=34}
	const float BondMul = (Req.Source == EExpSource::BondLevelUp) ? 1.0f : QueryBondMultiplierAndBroadcastIfChanged();

	// Travel은 “최근 의미있는 사건 없음” 감쇠 적용 :contentReference[oaicite:35]{index=35}
	float ActivityMul = 1.0f;
	if (Req.Source == EExpSource::Travel)
	{
		const double Now = Req.TimestampReal > 0.0 ? Req.TimestampReal : FPlatformTime::Seconds();
		ActivityMul = ComputeInactivityMultiplier(Now);
	}

	const float FinalF = FMath::FloorToFloat((float)BaseExpResolved * BondMul * ActivityMul);
	const int32 FinalExp = FMath::Max(0, (int32)FinalF);

	FExpGrantSnapshot Snap;
	Snap.Source = Req.Source;
	Snap.BaseExp = BaseExpResolved;
	Snap.FinalExp = FinalExp;
	Snap.BondMultiplierApplied = BondMul * ActivityMul;
	Snap.bUseGuidContext = Req.bUseGuidContext;
	Snap.ContextGuid = Req.ContextGuid;
	Snap.ContextName = Req.ContextName;
	Snap.TimestampReal = Req.TimestampReal;

	// 만렙 처리: 문서상 “EXP 추가는 허용하되 성장 정지(Clamp)” 옵션 중 A를 기본으로 :contentReference[oaicite:36]{index=36}
	if (PartyLevel >= LevelMax())
	{
		// 성장 정지: CurrentExp는 유지(Clamp)
		// 그래도 “지급 이벤트/텔레메트리”는 발생
		const FName CtxDbg = Req.bUseGuidContext ? FName(*Req.ContextGuid.ToString()) : Req.ContextName;
		OnExpGranted.Broadcast(Req.Source, BaseExpResolved, 0, CtxDbg);
		FlushToSave();
		return FExpGrantOp::Ok(Snap);
	}

	// 누적
	CurrentExp += FinalExp;

	// 레벨업 처리 while-loop (SSOT) :contentReference[oaicite:37]{index=37}
	while (PartyLevel < LevelMax())
	{
		const int32 Need = ExpToNextFor(PartyLevel);
		if (Need <= 0) break;

		if (CurrentExp >= Need)
		{
			CurrentExp -= Need;
			PartyLevel++;

			OnPartyLevelUp.Broadcast(PartyLevel); // SSOT 이벤트 :contentReference[oaicite:38]{index=38}
		}
		else break;
	}

	// 1회성 컨텍스트 저장 반영
	if (Req.Source == EExpSource::DiscoverArea)
	{
		DiscoveredAreas.Add(Req.ContextName);
		OnAreaDiscovered.Broadcast(Req.ContextName);
	}
	else if (Req.Source == EExpSource::DiscoverRestPoint)
	{
		DiscoveredRestPoints.Add(Req.ContextName);
		OnRestPointDiscovered.Broadcast(Req.ContextName);
	}
	else if (Req.Source == EExpSource::ExploreReward)
	{
		ClaimedExploreRewards.Add(Req.ContextGuid);
	}

	// OnExpGranted (SSOT) :contentReference[oaicite:39]{index=39}
	const FName CtxDbg = Req.bUseGuidContext ? FName(*Req.ContextGuid.ToString()) : Req.ContextName;
	OnExpGranted.Broadcast(Req.Source, BaseExpResolved, FinalExp, CtxDbg);

	FlushToSave();
	return FExpGrantOp::Ok(Snap);
}

FExpGrantOp ULevelingSubsystem::GrantExp(const FExpGrantRequest& Req)
{
	// Source별로 Context 요구(Discovery/ExploreReward) :contentReference[oaicite:40]{index=40}
	switch (Req.Source)
	{
	case EExpSource::DiscoverArea:
	case EExpSource::DiscoverRestPoint:
		if (Req.ContextName.IsNone()) return FExpGrantOp::Fail("Reject.InvalidContext");
		break;

	case EExpSource::ExploreReward:
		if (!Req.ContextGuid.IsValid()) return FExpGrantOp::Fail("Reject.InvalidContext");
		break;

	default:
		break;
	}

	// 기본값은 시스템에서 제공 가능(하지만 SSOT는 “각 시스템이 GrantExp 호출”) :contentReference[oaicite:41]{index=41}
	const int32 Base = Req.BaseExp;
	return ValidateAndApplyGrant(Req, Base);
}

// ---- Wrappers ----

FExpGrantOp ULevelingSubsystem::GrantTravelExp(int32 BaseExp, const FGuid& ContextGuid)
{
	FExpGrantRequest R;
	R.Source = EExpSource::Travel;
	R.BaseExp = BaseExp;
	R.bUseGuidContext = true;
	R.ContextGuid = ContextGuid;
	R.TimestampReal = FPlatformTime::Seconds();
	return GrantExp(R);
}

FExpGrantOp ULevelingSubsystem::GrantAreaDiscoveryExp(FName AreaId)
{
	FExpGrantRequest R;
	R.Source = EExpSource::DiscoverArea;
	R.BaseExp = ExpSettings ? ExpSettings->DiscoverAreaBaseExp : 30;
	R.bUseGuidContext = false;
	R.ContextName = AreaId;
	R.TimestampReal = FPlatformTime::Seconds();
	return GrantExp(R);
}

FExpGrantOp ULevelingSubsystem::GrantRestPointDiscoveryExp(FName RestPointId)
{
	FExpGrantRequest R;
	R.Source = EExpSource::DiscoverRestPoint;
	R.BaseExp = ExpSettings ? ExpSettings->DiscoverRestPointBaseExp : 30;
	R.bUseGuidContext = false;
	R.ContextName = RestPointId;
	R.TimestampReal = FPlatformTime::Seconds();
	return GrantExp(R);
}

FExpGrantOp ULevelingSubsystem::GrantExploreRewardExp(const FGuid& ExplorationObjectId)
{
	FExpGrantRequest R;
	R.Source = EExpSource::ExploreReward;
	R.BaseExp = ExpSettings ? ExpSettings->ExploreRewardBaseExp : 20;
	R.bUseGuidContext = true;
	R.ContextGuid = ExplorationObjectId;
	R.TimestampReal = FPlatformTime::Seconds();
	return GrantExp(R);
}

FExpGrantOp ULevelingSubsystem::GrantBondLevelUpExp(bool bIsTrioBond, const FGuid& BondContext)
{
	// BondLevelUp 발생 시 즉시 EXP 지급(SSOT) :contentReference[oaicite:42]{index=42}
	FExpGrantRequest R;
	R.Source = EExpSource::BondLevelUp;
	R.BaseExp = ExpSettings
		            ? (bIsTrioBond ? ExpSettings->BondTrioLevelUpBaseExp : ExpSettings->BondPairLevelUpBaseExp)
		            : (bIsTrioBond ? 30 : 10);
	R.bUseGuidContext = true;
	R.ContextGuid = BondContext;
	R.TimestampReal = FPlatformTime::Seconds();
	return GrantExp(R);
}

FExpGrantOp ULevelingSubsystem::GrantCombatRewardExp(int32 BaseExp, const FGuid& BattleSessionId)
{
	// 전투 Victory 시 지급 (SSOT) :contentReference[oaicite:43]{index=43}
	FExpGrantRequest R;
	R.Source = EExpSource::CombatReward;
	R.BaseExp = BaseExp;
	R.bUseGuidContext = true;
	R.ContextGuid = BattleSessionId;
	R.TimestampReal = FPlatformTime::Seconds();
	return GrantExp(R);
}
