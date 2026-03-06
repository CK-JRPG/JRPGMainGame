// Source/JRPGCombat/Private/Combat/Exploration/ExplorationSubsystem.cpp
#include "Combat/Exploration/ExplorationSubsystem.h"

#include "Combat/Exploration/RewardServiceSubsystem.h"
#include "Combat/Exploration/ExplorationObjectActor.h"
#include "Combat/Exploration/ExplorationObjectDataAsset.h"
#include "Combat/Exploration/ExplorationSaveGameSubsystem.h"
#include "Combat/Exploration/ExplorationProgressSubsystem.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UExplorationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Save/Progress 초기화
	if (UExplorationSaveGameSubsystem* SaveSys = GetSaveSys())
	{
		SaveSys->LoadOrCreate();
	}

	if (UExplorationProgressSubsystem* Prog = GetProgressSys())
	{
		if (UExplorationSaveGameSubsystem* SaveSys = GetSaveSys())
		{
			Prog->InitializeFromSave(SaveSys);
		}
	}

	// Respawn tick (간단: 1초마다)
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnTickHandle, this, &UExplorationSubsystem::TickRespawns, 1.0f,
		                                       true);
	}
}

double UExplorationSubsystem::NowReal() const
{
	return GetWorld() ? (double)GetWorld()->GetRealTimeSeconds() : 0.0;
}

URewardServiceSubsystem* UExplorationSubsystem::GetRewardService() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<URewardServiceSubsystem>() : nullptr;
}

UExplorationSaveGameSubsystem* UExplorationSubsystem::GetSaveSys() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UExplorationSaveGameSubsystem>()
		       : nullptr;
}

UExplorationProgressSubsystem* UExplorationSubsystem::GetProgressSys() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<UExplorationProgressSubsystem>()
		       : nullptr;
}

void UExplorationSubsystem::RegisterObject(AExplorationObjectActor* Obj)
{
	if (!Obj) return;
	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data || !Data->IsValidObject()) return;

	ObjectMap.Add(Data->ObjectId, Obj);

	ApplyInitialStateToActor(Obj, *Data);
}

void UExplorationSubsystem::UnregisterObject(AExplorationObjectActor* Obj)
{
	if (!Obj) return;
	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data) return;

	ObjectMap.Remove(Data->ObjectId);
}

bool UExplorationSubsystem::TryGetObjectActor(const FGuid& ObjectId, TWeakObjectPtr<AExplorationObjectActor>& Out) const
{
	if (const TWeakObjectPtr<AExplorationObjectActor>* Found = ObjectMap.Find(ObjectId))
	{
		Out = *Found;
		return Out.IsValid();
	}
	return false;
}

void UExplorationSubsystem::GetAllRegisteredObjects(TArray<TWeakObjectPtr<AExplorationObjectActor>>& Out) const
{
	Out.Reset();
	Out.Reserve(ObjectMap.Num());
	for (const auto& KV : ObjectMap)
	{
		Out.Add(KV.Value);
	}
}

EExplorationObjectState UExplorationSubsystem::GetObjectState(const FGuid& ObjectId) const
{
	const UExplorationSaveGameSubsystem* SaveSys = GetSaveSys();
	const UExplorationSaveGame* Save = SaveSys ? SaveSys->GetSave() : nullptr;

	if (Save && Save->CompletedOneTimeObjects.Contains(ObjectId))
		return EExplorationObjectState::CompletedOneTime;

	if (Save)
	{
		if (const double* T = Save->RespawnAvailableAtReal.Find(ObjectId))
		{
			if (NowReal() < *T)
				return EExplorationObjectState::WaitingRespawn;
		}
	}
	return EExplorationObjectState::Active;
}

void UExplorationSubsystem::ApplyInitialStateToActor(AExplorationObjectActor* Obj,
                                                     const UExplorationObjectDataAsset& Data)
{
	if (!Obj) return;

	const EExplorationObjectState S = GetObjectState(Data.ObjectId);

	if (S == EExplorationObjectState::CompletedOneTime)
	{
		Obj->SetExplorationActive(false);
		return;
	}

	if (S == EExplorationObjectState::WaitingRespawn)
	{
		Obj->SetExplorationActive(false);
		return;
	}

	Obj->SetExplorationActive(true);
}

void UExplorationSubsystem::TickRespawns()
{
	UExplorationSaveGameSubsystem* SaveSys = GetSaveSys();
	UExplorationSaveGame* Save = SaveSys ? SaveSys->GetSave() : nullptr;
	if (!Save) return;

	const double Now = NowReal();

	// Respawn 가능한 오브젝트를 다시 활성화
	TArray<FGuid> ToActivate;
	for (const auto& KV : Save->RespawnAvailableAtReal)
	{
		const FGuid& ObjectId = KV.Key;
		const double AvailableAt = KV.Value;
		if (Now >= AvailableAt)
		{
			ToActivate.Add(ObjectId);
		}
	}

	for (const FGuid& ObjectId : ToActivate)
	{
		Save->RespawnAvailableAtReal.Remove(ObjectId);
		SaveSys->MarkDirty();

		TWeakObjectPtr<AExplorationObjectActor> Obj;
		if (TryGetObjectActor(ObjectId, Obj))
		{
			if (Obj.IsValid())
			{
				Obj->SetExplorationActive(true);
			}
		}
	}
}

FExplorationOp UExplorationSubsystem::CheckLock(const UExplorationObjectDataAsset& Data) const
{
	const FExplorationLockCondition& L = Data.LockCondition;
	if (L.LockType == EExplorationLockType::None)
		return FExplorationOp::Ok();

	// KeyItem / Flag만 최소 구현. QuestState는 추후 퀘스트 시스템 연동.
	if (L.LockType == EExplorationLockType::KeyItem)
	{
		if (L.RequiredItemId.IsNone())
			return FExplorationOp::Fail("Reject.Lock.InvalidKeyItem");

		// Inventory에 KeyItem이 있는지 체크
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (UInventorySubsystem* Inv = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>())
			{
				if (!Inv->HasItem(L.RequiredItemId, 1))
					return FExplorationOp::Fail("Reject.Lock.MissingKeyItem");
			}
			else return FExplorationOp::Fail("Reject.Lock.NoInventory");
		}
		return FExplorationOp::Ok();
	}

	if (L.LockType == EExplorationLockType::Flag)
	{
		if (L.RequiredFlagId.IsNone())
			return FExplorationOp::Fail("Reject.Lock.InvalidFlag");

		if (UExplorationProgressSubsystem* P = GetProgressSys())
		{
			if (!P->HasFlag(L.RequiredFlagId))
				return FExplorationOp::Fail("Reject.Lock.MissingFlag");
		}
		else return FExplorationOp::Fail("Reject.Lock.NoProgress");

		return FExplorationOp::Ok();
	}

	if (L.LockType == EExplorationLockType::QuestState)
	{
		return FExplorationOp::Fail("Reject.Lock.QuestNotImplemented");
	}

	return FExplorationOp::Ok();
}

FExplorationOp UExplorationSubsystem::CompleteObject(AActor* Instigator, const UExplorationObjectDataAsset& Data,
                                                     EExplorationTriggerType TriggerType)
{
	// RewardService 단일 관문 :contentReference[oaicite:29]{index=29}
	URewardServiceSubsystem* RewardService = GetRewardService();
	if (!RewardService) return FExplorationOp::Fail("Reject.NoRewardService");

	// reward request
	FRewardGrantRequest Req;
	Req.SourceObjectId = Data.ObjectId;
	Req.SourceTag = "Explore.Reward"; // 문서 텔레메트리 기준 :contentReference[oaicite:30]{index=30}
	Req.TriggerType = TriggerType;
	Req.bOneTimeContext = (Data.AcquisitionPolicy == EExplorationAcquisitionPolicy::OneTime);
	Req.Table = Data.RewardTable;
	Req.DirectEntries = Data.RewardEntries;
	Req.Instigator = Instigator;

	TArray<FGrantedReward> Granted;
	const FExplorationOp GrantOp = RewardService->GrantRewards(Req, Granted);

	// 이벤트 발행: OnRewardsGranted(SourceId, RewardList) :contentReference[oaicite:31]{index=31}
	OnRewardsGranted.Broadcast(Data.ObjectId, Granted);

	// 저장 반영
	if (UExplorationSaveGameSubsystem* SaveSys = GetSaveSys())
	{
		if (UExplorationSaveGame* Save = SaveSys->GetSave())
		{
			if (Data.AcquisitionPolicy == EExplorationAcquisitionPolicy::OneTime)
			{
				Save->CompletedOneTimeObjects.Add(Data.ObjectId);
				SaveSys->MarkDirty();
			}
			else if (Data.AcquisitionPolicy == EExplorationAcquisitionPolicy::Respawn)
			{
				const double AvailableAt = NowReal() + FMath::Max(0.f, Data.RespawnTimeSec);
				Save->RespawnAvailableAtReal.Add(Data.ObjectId, AvailableAt);
				SaveSys->MarkDirty();
			}
		}
	}

	return GrantOp.bOk ? FExplorationOp::Ok() : GrantOp;
}

FExplorationOp UExplorationSubsystem::TryInteract(AActor* Interactor, const FGuid& ObjectId)
{
	TWeakObjectPtr<AExplorationObjectActor> Obj;
	if (!TryGetObjectActor(ObjectId, Obj))
	{
		OnExplorationObjectCompleted.Broadcast(ObjectId, EExplorationOpResult::Rejected, "Reject.ObjectNotFound");
		return FExplorationOp::Fail("Reject.ObjectNotFound");
	}

	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data)
	{
		OnExplorationObjectCompleted.Broadcast(ObjectId, EExplorationOpResult::Rejected, "Reject.NoData");
		return FExplorationOp::Fail("Reject.NoData");
	}

	// 상태 체크(1회성/리스폰)
	const EExplorationObjectState S = GetObjectState(ObjectId);
	if (S != EExplorationObjectState::Active)
	{
		OnExplorationObjectCompleted.Broadcast(ObjectId, EExplorationOpResult::Rejected, "Reject.NotActive");
		return FExplorationOp::Fail("Reject.NotActive");
	}

	// LockCondition 체크 :contentReference[oaicite:32]{index=32}
	const FExplorationOp Lock = CheckLock(*Data);
	if (!Lock.bOk)
	{
		OnExplorationObjectCompleted.Broadcast(ObjectId, EExplorationOpResult::Rejected, Lock.ReasonTag);
		return Lock;
	}

	// 성공 처리: Reward + Save + 완료 이벤트 :contentReference[oaicite:33]{index=33}
	const FExplorationOp Done = CompleteObject(Interactor, *Data, EExplorationTriggerType::Interact);

	// Actor 비활성화
	if (Data->AcquisitionPolicy == EExplorationAcquisitionPolicy::OneTime ||
		Data->AcquisitionPolicy == EExplorationAcquisitionPolicy::Respawn)
	{
		Obj->SetExplorationActive(false);
	}

	OnExplorationObjectCompleted.Broadcast(
		ObjectId, Done.bOk ? EExplorationOpResult::Success : EExplorationOpResult::Rejected,
		Done.bOk ? "Success" : Done.ReasonTag);
	return Done;
}

FExplorationOp UExplorationSubsystem::NotifySolved(AActor* Instigator, const FGuid& ObjectId)
{
	// Solve는 퍼즐 시스템에서 호출(성공 시 Reward 지급)
	TWeakObjectPtr<AExplorationObjectActor> Obj;
	if (!TryGetObjectActor(ObjectId, Obj)) return FExplorationOp::Fail("Reject.ObjectNotFound");
	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data) return FExplorationOp::Fail("Reject.NoData");

	const EExplorationObjectState S = GetObjectState(ObjectId);
	if (S != EExplorationObjectState::Active) return FExplorationOp::Fail("Reject.NotActive");

	const FExplorationOp Done = CompleteObject(Instigator, *Data, EExplorationTriggerType::Solve);
	if (Data->AcquisitionPolicy != EExplorationAcquisitionPolicy::Respawn)
		Obj->SetExplorationActive(false);

	OnExplorationObjectCompleted.Broadcast(
		ObjectId, Done.bOk ? EExplorationOpResult::Success : EExplorationOpResult::Rejected,
		Done.bOk ? "Success" : Done.ReasonTag);
	return Done;
}

FExplorationOp UExplorationSubsystem::NotifyDefeated(AActor* Instigator, const FGuid& ObjectId)
{
	// Defeat는 전투 시스템이 호출(드랍/고정보상) :contentReference[oaicite:34]{index=34}
	TWeakObjectPtr<AExplorationObjectActor> Obj;
	if (!TryGetObjectActor(ObjectId, Obj)) return FExplorationOp::Fail("Reject.ObjectNotFound");
	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data) return FExplorationOp::Fail("Reject.NoData");

	const EExplorationObjectState S = GetObjectState(ObjectId);
	if (S != EExplorationObjectState::Active) return FExplorationOp::Fail("Reject.NotActive");

	const FExplorationOp Done = CompleteObject(Instigator, *Data, EExplorationTriggerType::Defeat);
	if (Data->AcquisitionPolicy != EExplorationAcquisitionPolicy::Respawn)
		Obj->SetExplorationActive(false);

	OnExplorationObjectCompleted.Broadcast(
		ObjectId, Done.bOk ? EExplorationOpResult::Success : EExplorationOpResult::Rejected,
		Done.bOk ? "Success" : Done.ReasonTag);
	return Done;
}

FExplorationOp UExplorationSubsystem::TryDiscover(FName DiscoveryId, AActor* Discoverer,
                                                  UExplorationRewardTableAsset* OptionalRewardTable)
{
	if (DiscoveryId.IsNone())
		return FExplorationOp::Fail("Reject.InvalidDiscoveryId");

	UExplorationSaveGameSubsystem* SaveSys = GetSaveSys();
	UExplorationSaveGame* Save = SaveSys ? SaveSys->GetSave() : nullptr;
	if (!Save) return FExplorationOp::Fail("Reject.NoSave");

	FDiscoveryRecord& R = Save->DiscoveryMap.FindOrAdd(DiscoveryId);

	if (R.bDiscovered)
	{
		// 중복 방지(SSOT) :contentReference[oaicite:35]{index=35}
		return FExplorationOp::Fail("Reject.DuplicateDiscovery");
	}

	R.bDiscovered = true;
	R.DiscoveredAtRealTime = NowReal();
	SaveSys->MarkDirty();

	OnDiscoveryChanged.Broadcast(DiscoveryId, true);

	// 발견 보상(선택): “필요 시 소량의 발견 보상” :contentReference[oaicite:36]{index=36}
	if (OptionalRewardTable)
	{
		if (URewardServiceSubsystem* RewardService = GetRewardService())
		{
			FRewardGrantRequest Req;
			Req.SourceDiscoveryId = DiscoveryId;
			Req.SourceTag = "Explore.DiscoveryReward";
			Req.TriggerType = EExplorationTriggerType::Discovery;
			Req.bOneTimeContext = true;
			Req.Table = OptionalRewardTable;
			Req.Instigator = Discoverer;

			TArray<FGrantedReward> Granted;
			const FExplorationOp GrantOp = RewardService->GrantRewards(Req, Granted);

			// SourceObjectId가 없으므로 “가짜 GUID” 대신 0 GUID로 발행(레벨업 시스템은 DiscoveryId로도 판정 가능)
			OnRewardsGranted.Broadcast(FGuid(), Granted);

			if (GrantOp.bOk)
			{
				R.bOptionalRewardGranted = true;
				SaveSys->MarkDirty();
			}
		}
	}

	return FExplorationOp::Ok();
}

void UExplorationSubsystem::NotifyPromptChanged(const FGuid& ObjectId, bool bVisible)
{
	OnInteractPromptChanged.Broadcast(ObjectId, bVisible);
}
