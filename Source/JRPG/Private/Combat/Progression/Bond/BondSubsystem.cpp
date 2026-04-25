// Source/JRPGCombat/Private/Combat/Progression/Bond/BondSubsystem.cpp
#include "Combat/Progression/Bond/BondSubsystem.h"
#include "Combat/Progression/Bond/BondSaveGameSubsystem.h"
#include "Combat/Progression/Leveling/LevelingSubsystem.h"

#include"Combat/Exploration/ExplorationProgressSubsystem.h"// RequiredFlags를 WorldFlags로 소비(확장 슬롯)

double UBondSubsystem::NowReal() const
{
	return FPlatformTime::Seconds();
}

const UBondSettingsDataAsset* UBondSubsystem::GetSettings() const
{
	return Settings ? Settings : nullptr;
}

UBondSaveGameSubsystem* UBondSubsystem::GetSaveSys() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UBondSaveGameSubsystem>() : nullptr;
}

void UBondSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadFromSave();

	// 레벨업 시스템이 BondExpBonusProvider를 조회할 수 있게 연결(자동)
	if (ULevelingSubsystem* L = GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
	{
		L->BondBonusProvider = TScriptInterface<IBondExpBonusProvider>(this);
	}

	RecomputePartyTrioLevelAndBonus(false);
	EvaluateDialogueUnlocks();
}

void UBondSubsystem::LoadFromSave()
{
	if (UBondSaveGameSubsystem* SaveSys = GetSaveSys())
	{
		SaveSys->LoadOrCreate();
		if (UBondSaveGame* S = SaveSys->GetSave())
		{
			PairStates = S->PairStates;
			TrioStates = S->TrioStates;
			UnlockedDialogueNodes = S->UnlockedDialogueNodes;
			CompletedDialogueNodes = S->CompletedDialogueNodes;

			CurrentPartyIds = S->CurrentPartyIds;
			LastSignificantProgressReal = S->LastSignificantProgressReal;

			CachedTrioLevel = FMath::Clamp(S->CachedTrioLevelForParty, 1, 5);
			CachedExpBonusMultiplier = FMath::Max(0.f, S->CachedExpBonusMultiplier);
		}
	}

	// 기본 파티가 없으면 빈 상태(전투 캐릭터 시스템에서 SetCurrentParty를 호출하게 됨)
}

void UBondSubsystem::FlushToSave()
{
	if (UBondSaveGameSubsystem* SaveSys = GetSaveSys())
	{
		if (UBondSaveGame* S = SaveSys->GetSave())
		{
			S->PairStates = PairStates;
			S->TrioStates = TrioStates;

			S->UnlockedDialogueNodes = UnlockedDialogueNodes;
			S->CompletedDialogueNodes = CompletedDialogueNodes;

			S->CurrentPartyIds = CurrentPartyIds;
			S->LastSignificantProgressReal = LastSignificantProgressReal;

			S->CachedTrioLevelForParty = CachedTrioLevel;
			S->CachedExpBonusMultiplier = CachedExpBonusMultiplier;

			SaveSys->MarkDirty();
		}
	}
}

bool UBondSubsystem::ValidateParticipants(const TArray<FName>& P, FName& OutReason) const
{
	OutReason = NAME_None;

	if (P.Num() != 2 && P.Num() != 3)
	{
		OutReason = "Reject.InvalidParticipants"; // SSOT :contentReference[oaicite:33]{index=33}
		return false;
	}

	TSet<FName> S;
	for (const FName& X : P)
	{
		if (X.IsNone())
		{
			OutReason = "Reject.InvalidParticipants";
			return false;
		}
		S.Add(X);
	}
	if (S.Num() != P.Num())
	{
		OutReason = "Reject.InvalidParticipants";
		return false;
	}

	return true;
}

float UBondSubsystem::ComputeInactivityMul(double Now) const
{
	const UBondSettingsDataAsset* S = GetSettings();
	if (!S) return 1.0f;

	const float W = FMath::Max(0.f, S->InactivityWindowSec);
	if (W <= 0.f) return 1.0f;

	const double Dt = Now - LastSignificantProgressReal;
	if (Dt <= (double)W) return 1.0f;

	// “유의미 진행 없으면 획득 속도 감소” :contentReference[oaicite:34]{index=34}
	return FMath::Clamp(S->InactivityMul, 0.f, 1.f);
}

FBondState& UBondSubsystem::GetOrInitPairState(const FBondPairId& Id)
{
	if (FBondState* Found = PairStates.Find(Id))return *Found;
	return PairStates.Add(Id, FBondState::Default());
}

FBondState& UBondSubsystem::GetOrInitTrioState(const FBondTrioId& Id)
{
	if (FBondState* Found = TrioStates.Find(Id))return *Found;
	return TrioStates.Add(Id, FBondState::Default());
}

FBondState UBondSubsystem::GetBondState_Pair(const FBondPairId& BondId) const
{
	if (const FBondState* S = PairStates.Find(BondId))return *S;
	return FBondState::Default();
}

FBondState UBondSubsystem::GetBondState_Trio(const FBondTrioId& BondId) const
{
	if (const FBondState* S = TrioStates.Find(BondId))return *S;
	return FBondState::Default();
}

FBondOp UBondSubsystem::SetCurrentParty(const TArray<FName>& PartyIds)
{
	if (PartyIds.Num() <= 0 || PartyIds.Num() > 3)
		return FBondOp::Fail("Reject.InvalidParticipants");

	TSet<FName> S;
	for (const FName& P : PartyIds)
	{
		if (P.IsNone())return FBondOp::Fail("Reject.InvalidParticipants");
		S.Add(P);
	}
	if (S.Num() != PartyIds.Num())return FBondOp::Fail("Reject.InvalidParticipants");

	CurrentPartyIds = PartyIds;

	RecomputePartyTrioLevelAndBonus(true);
	EvaluateDialogueUnlocks();
	FlushToSave();
	return FBondOp::Ok();
}

void UBondSubsystem::NotifySignificantProgress(FName/*EventTag*/)
{
	LastSignificantProgressReal = NowReal();
	FlushToSave();
}

int32 UBondSubsystem::GetTrioBondLevelForCurrentParty() const
{
	return CachedTrioLevel;
}

float UBondSubsystem::GetExpBonusMultiplierForCurrentParty() const
{
	return CachedExpBonusMultiplier;
}

void UBondSubsystem::RecomputePartyTrioLevelAndBonus(bool bBroadcastIfChanged)
{
	const UBondSettingsDataAsset* S = GetSettings();
	if (!S)
	{
		CachedTrioLevel = 1;
		CachedExpBonusMultiplier = 1.0f;
		return;
	}

	if (CurrentPartyIds.Num() != 3)
	{
		CachedTrioLevel = 1;
		CachedExpBonusMultiplier = S->GetExpBonusMulByTrioLevel(1);
		return;
	}

	const FName A = CurrentPartyIds[0];
	const FName B = CurrentPartyIds[1];
	const FName C = CurrentPartyIds[2];

	// SSOT 예시: TrioBondLevel = floor((BL_AB + BL_BC + BL_AC)/3) :contentReference[oaicite:35]{index=35}
	const int32 BL_AB = GetBondState_Pair(FBondPairId::Make(A, B)).BondLevel;
	const int32 BL_BC = GetBondState_Pair(FBondPairId::Make(B, C)).BondLevel;
	const int32 BL_AC = GetBondState_Pair(FBondPairId::Make(A, C)).BondLevel;

	const int32 NewTrioLevel = FMath::Clamp((BL_AB + BL_BC + BL_AC) / 3, 1, 5);
	const float NewMul = S->GetExpBonusMulByTrioLevel(NewTrioLevel); // :contentReference[oaicite:36]{index=36}

	const bool bTrioChanged = (NewTrioLevel != CachedTrioLevel);
	const bool bMulChanged = !FMath::IsNearlyEqual(NewMul, CachedExpBonusMultiplier, 0.0001f);

	CachedTrioLevel = NewTrioLevel;
	CachedExpBonusMultiplier = NewMul;

	if (bBroadcastIfChanged && (bTrioChanged || bMulChanged))
	{
		// 트리오 레벨 변화 시 OnBondExpBonusChanged 발행(SSOT) :contentReference[oaicite:37]{index=37}
		OnBondExpBonusChanged.Broadcast(CachedExpBonusMultiplier);
	}

	FlushToSave();
}

void UBondSubsystem::NotifyLevelingSystem_BondLevelUp(bool bIsTrioBond)
{
	// 레벨업 시스템 규칙: BondLevelUp 이벤트 발생 시 즉시 EXP 지급, 페어/트리오 차등 :contentReference[oaicite:38]{index=38}
	if (ULevelingSubsystem* L = GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
	{
		L->GrantBondLevelUpExp(bIsTrioBond, FGuid::NewGuid());
	}
}

FBondOp UBondSubsystem::AddToPairBond(const FBondPairId& Id, EBondSource Source, int32 BaseAmount,
                                      const FBondAddRequest& Req)
{
	const UBondSettingsDataAsset* S = GetSettings();
	if (!S)return FBondOp::Fail("Reject.InvalidParticipants");
	if (!Id.IsValid())return FBondOp::Fail("Reject.InvalidParticipants");

	FBondState& St = GetOrInitPairState(Id);

	if (St.BondLevel >= 5)
		return FBondOp::Fail("Reject.MaxLevel"); // SSOT :contentReference[oaicite:39]{index=39}

	// 쿨다운(악용 방지) :contentReference[oaicite:40]{index=40}
	const double Now = NowReal();
	if ((Now - St.LastBPEventTimeReal) < (double)S->AntiExploitCooldownSec)
		return FBondOp::Fail("Reject.AntiExploitCooldown");

	// Walk는 유의미 진행 없으면 속도 감소(감쇠) :contentReference[oaicite:41]{index=41}
	float ActivityMul = 1.0f;
	if (Source == EBondSource::Walk)
		ActivityMul = ComputeInactivityMul(Now);
	else
		NotifySignificantProgress("Bond.NonWalk"); // 전투/휴식은 유의미 진행으로 간주

	const float Diminish = S->GetDiminishMul(St.BondLevel); // 레벨별 감쇠 :contentReference[oaicite:42]{index=42}
	const int32 Gain = FMath::Max(0, (int32)FMath::FloorToInt((float)BaseAmount * Diminish * ActivityMul));

	St.BondPoint += Gain;
	St.TotalEarnedBP += Gain;
	St.LastBPEventTimeReal = Now;

	OnBondPointsGained.Broadcast(Id.ToDebugName(), Gain, Req.SourceTag); // SSOT :contentReference[oaicite:43]{index=43}

	// 자동 레벨업 가능(SSOT) :contentReference[oaicite:44]{index=44}
	TryAutoLevelUpPair(Id);

	FlushToSave();
	return FBondOp::Ok();
}

FBondOp UBondSubsystem::AddToTrioBond(const FBondTrioId& Id, EBondSource Source, int32 BaseAmount,
                                      const FBondAddRequest& Req)
{
	const UBondSettingsDataAsset* S = GetSettings();
	if (!S)return FBondOp::Fail("Reject.InvalidParticipants");
	if (!Id.IsValid())return FBondOp::Fail("Reject.InvalidParticipants");

	FBondState& St = GetOrInitTrioState(Id);

	if (St.BondLevel >= 5)
		return FBondOp::Fail("Reject.MaxLevel");

	const double Now = NowReal();
	if ((Now - St.LastBPEventTimeReal) < (double)S->AntiExploitCooldownSec)
		return FBondOp::Fail("Reject.AntiExploitCooldown");

	float ActivityMul = 1.0f;
	if (Source == EBondSource::Walk)
		ActivityMul = ComputeInactivityMul(Now);
	else
		NotifySignificantProgress("Bond.NonWalk");

	const float Diminish = S->GetDiminishMul(St.BondLevel);
	const int32 Gain = FMath::Max(0, (int32)FMath::FloorToInt((float)BaseAmount * Diminish * ActivityMul));

	St.BondPoint += Gain;
	St.TotalEarnedBP += Gain;
	St.LastBPEventTimeReal = Now;

	OnBondPointsGained.Broadcast(Id.ToDebugName(), Gain, Req.SourceTag);

	TryAutoLevelUpTrio(Id);

	FlushToSave();
	return FBondOp::Ok();
}

bool UBondSubsystem::TryAutoLevelUpPair(const FBondPairId& Id)
{
	FBondState& St = GetOrInitPairState(Id);
	bool bAny = false;

	// BP>=100이면 레벨업 가능, 필요 BP 고정 100, 레벨업 시 -100 :contentReference[oaicite:45]{index=45}
	while (St.BondLevel < 5 && St.BondPoint >= 100)
	{
		St.BondPoint -= 100;
		St.BondLevel++;
		bAny = true;

		OnBondLevelUp.Broadcast(Id.ToDebugName(), St.BondLevel); // SSOT :contentReference[oaicite:46]{index=46}
		NotifyLevelingSystem_BondLevelUp(false);
	}

	if (bAny)
	{
		RecomputePartyTrioLevelAndBonus(true);
		EvaluateDialogueUnlocks();
	}
	return bAny;
}

bool UBondSubsystem::TryAutoLevelUpTrio(const FBondTrioId& Id)
{
	FBondState& St = GetOrInitTrioState(Id);
	bool bAny = false;

	while (St.BondLevel < 5 && St.BondPoint >= 100)
	{
		St.BondPoint -= 100;
		St.BondLevel++;
		bAny = true;

		OnBondLevelUp.Broadcast(Id.ToDebugName(), St.BondLevel);
		NotifyLevelingSystem_BondLevelUp(true);
	}

	if (bAny)
	{
		RecomputePartyTrioLevelAndBonus(true);
		EvaluateDialogueUnlocks();
	}
	return bAny;
}

FBondOp UBondSubsystem::TryLevelUpBond_Pair(const FBondPairId& BondId)
{
	FBondState& St = GetOrInitPairState(BondId);

	if (St.BondLevel >= 5)return FBondOp::Fail("Reject.MaxLevel");
	if (St.BondPoint < 100)return FBondOp::Fail("Reject.NotEnoughBP"); // SSOT :contentReference[oaicite:47]{index=47}

	St.BondPoint -= 100;
	St.BondLevel++;

	OnBondLevelUp.Broadcast(BondId.ToDebugName(), St.BondLevel);
	NotifyLevelingSystem_BondLevelUp(false);

	RecomputePartyTrioLevelAndBonus(true);
	EvaluateDialogueUnlocks();
	FlushToSave();
	return FBondOp::Ok();
}

FBondOp UBondSubsystem::TryLevelUpBond_Trio(const FBondTrioId& BondId)
{
	FBondState& St = GetOrInitTrioState(BondId);

	if (St.BondLevel >= 5)return FBondOp::Fail("Reject.MaxLevel");
	if (St.BondPoint < 100)return FBondOp::Fail("Reject.NotEnoughBP");

	St.BondPoint -= 100;
	St.BondLevel++;

	OnBondLevelUp.Broadcast(BondId.ToDebugName(), St.BondLevel);
	NotifyLevelingSystem_BondLevelUp(true);

	RecomputePartyTrioLevelAndBonus(true);
	EvaluateDialogueUnlocks();
	FlushToSave();
	return FBondOp::Ok();
}

FBondOp UBondSubsystem::AddBondPoints(const FBondAddRequest& Req)
{
	FName Reason;
	if (!ValidateParticipants(Req.Participants, Reason))
		return FBondOp::Fail(Reason);

	const UBondSettingsDataAsset* S = GetSettings();
	if (!S)return FBondOp::Fail("Reject.InvalidParticipants");

	if (Req.BaseAmount <= 0)
		return FBondOp::Fail("Reject.InvalidParticipants");

	// Participants 정렬/정규화
	TArray<FName> P = Req.Participants;
	P.Sort([](const FName& L, const FName& R) { return L.ToString() < R.ToString(); });

	// Pair
	if (P.Num() == 2)
	{
		const FBondPairId Id = FBondPairId::Make(P[0], P[1]);
		return AddToPairBond(Id, Req.Source, Req.BaseAmount, Req);
	}

	// Trio
	const FBondTrioId TrioId = FBondTrioId::Make(P[0], P[1], P[2]);

	// 기본 정책(밸런스 안전): 트리오 이벤트면
	// 1) 트리오 본드에 1회 지급
	// 2) 설정에 따라 페어 3개에 “분배 지급”(총합이 과해지는 3배를 막음)
	FBondOp TrioOp = AddToTrioBond(TrioId, Req.Source, Req.BaseAmount, Req);

	if (S->bDistributeTrioEventToPairs)
	{
		const int32 Total = Req.BaseAmount;
		const int32 BaseEach = Total / 3;
		const int32 Rem = Total % 3;

		const TArray<FBondPairId> Pairs = {
			FBondPairId::Make(P[0], P[1]),
			FBondPairId::Make(P[0], P[2]),
			FBondPairId::Make(P[1], P[2]),
		};

		for (int32 i = 0; i < 3; ++i)
		{
			const int32 Each = BaseEach + (i < Rem ? 1 : 0);
			if (Each <= 0)continue;

			FBondAddRequest PairReq = Req;
			PairReq.Participants = {Pairs[i].A, Pairs[i].B};
			PairReq.BaseAmount = Each;
			AddToPairBond(Pairs[i], Req.Source, Each, PairReq);
		}
	}

	return TrioOp.bOk ? FBondOp::Ok() : TrioOp;
}

UBondDialogueNodeDataAsset* UBondSubsystem::FindNode(FName NodeId) const
{
	for (UBondDialogueNodeDataAsset* N : DialogueNodes)
	{
		if (N && N->NodeId == NodeId) return N;
	}
	return nullptr;
}

bool UBondSubsystem::AreFlagsSatisfied(const TArray<FName>& Flags) const
{
	if (Flags.Num() == 0) return true;

	// RequiredFlags는 “진행 플래그(지역/보스 등)”를 의미 :contentReference[oaicite:48]{index=48}
	// 현재 프로젝트에선 ExplorationProgressSubsystem(WorldFlags)를 최소 연결로 사용(추후 Story/Quest로 교체 가능)
	if (UExplorationProgressSubsystem* P = GetGameInstance()->GetSubsystem<UExplorationProgressSubsystem>())
	{
		for (const FName& F : Flags)
		{
			if (!P->HasFlag(F)) return false;
		}
		return true;
	}

	// 플래그 시스템이 아직 없으면 “보수적으로 잠금”
	return false;
}

int32 UBondSubsystem::GetBondLevelForNode(const UBondDialogueNodeDataAsset& Node) const
{
	TArray<FName> P = Node.Participants;
	P.Sort([](const FName& L, const FName& R) { return L.ToString() < R.ToString(); });

	if (P.Num() == 2)
	{
		return GetBondState_Pair(FBondPairId::Make(P[0], P[1])).BondLevel;
	}

	// 트리오 대화는 “현재 파티 트리오 레벨(평균)”로 판단(문서: 트리오 대화는 3/5 구간) :contentReference[oaicite:49]{index=49}
	return CachedTrioLevel;
}

void UBondSubsystem::EvaluateDialogueUnlocks()
{
	for (UBondDialogueNodeDataAsset* Node : DialogueNodes)
	{
		if (!Node || !Node->IsValidNode())continue;
		if (CompletedDialogueNodes.Contains(Node->NodeId))continue;

		if (!AreFlagsSatisfied(Node->RequiredFlags))continue;

		const int32 L = GetBondLevelForNode(*Node);
		if (L < Node->MinBondLevel)continue;

		if (!UnlockedDialogueNodes.Contains(Node->NodeId))
		{
			UnlockedDialogueNodes.Add(Node->NodeId);
			OnBondDialogueUnlocked.Broadcast(Node->NodeId); // SSOT :contentReference[oaicite:50]{index=50}
		}
	}

	FlushToSave();
}

FBondOp UBondSubsystem::CompleteDialogue(FName NodeId)
{
	UBondDialogueNodeDataAsset* Node = FindNode(NodeId);
	if (!Node || !Node->IsValidNode())
		return FBondOp::Fail("Reject.InvalidParticipants");

	if (!UnlockedDialogueNodes.Contains(NodeId))
		return FBondOp::Fail("Reject.InvalidParticipants");

	if (CompletedDialogueNodes.Contains(NodeId))
		return FBondOp::Fail("Reject.AntiExploitCooldown");

	const UBondSettingsDataAsset* S = GetSettings();
	if (!S)return FBondOp::Fail("Reject.InvalidParticipants");

	// 규칙: 휴식 대화 1회 완료 시 BP 지급 :contentReference[oaicite:51]{index=51}
	const int32 Amount = FMath::Max(0, S->RestTalkBPBase + Node->RewardBP);
	// Flow: BaseBP + NodeBonus :contentReference[oaicite:52]{index=52}

	FBondAddRequest R;
	R.Source = EBondSource::RestTalk;
	R.Participants = Node->Participants;
	R.BaseAmount = Amount;
	R.Context = NodeId;
	R.SourceTag = "Bond.Dialogue.Played"; // 텔레메트리 항목 :contentReference[oaicite:53]{index=53}

	const FBondOp Op = AddBondPoints(R);

	CompletedDialogueNodes.Add(NodeId);
	FlushToSave();

	// 대화 완료 이벤트(SSOT) :contentReference[oaicite:54]{index=54}
	// 대표 BondId는 “참여자 기준”으로 생성(페어면 페어ID, 트리오면 트리오ID)
	FName BondId = NAME_None;
	if (Node->Participants.Num() == 2)
	{
		BondId = FBondPairId::Make(Node->Participants[0], Node->Participants[1]).ToDebugName();
	}
	else
	{
		const FBondTrioId T = FBondTrioId::Make(Node->Participants[0], Node->Participants[1], Node->Participants[2]);
		BondId = T.ToDebugName();
	}
	OnBondDialogueCompleted.Broadcast(NodeId, BondId, Amount);

	RecomputePartyTrioLevelAndBonus(true);
	EvaluateDialogueUnlocks();
	return Op;
}
