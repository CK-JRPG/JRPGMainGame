#include "Combat/Exploration/ExplorationSubsystem.h"

#include "Combat/Exploration/RewardServiceSubsystem.h"
#include "Combat/Exploration/ExplorationObjectActor.h"
#include "Combat/Exploration/ExplorationObjectDataAsset.h"
#include "Combat/Exploration/ExplorationSaveGameSubsystem.h"
#include "Combat/Exploration/ExplorationProgressSubsystem.h"
#include "Combat/Exploration/ExplorationRewardTableAsset.h"

#include "Combat/Items/InventorySubsystem.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UExplorationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Save/Progress 준비
	if (UExplorationSaveGameSubsystem* SaveSys = GetSaveSys())
	{
		if (!SaveSys->GetSave())
			SaveSys->LoadOrCreate();
	}

	if (UExplorationProgressSubsystem* Prog = GetProgressSys())
	{
		if (UExplorationSaveGameSubsystem* SaveSys = GetSaveSys())
			Prog->InitializeFromSave(SaveSys);
	}

	// Respawn tick (1초)
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
	for (const auto& KV : ObjectMap)Out.Add(KV.Value);
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
	Obj->SetExplorationActive(S == EExplorationObjectState::Active);
}

void UExplorationSubsystem::TickRespawns()
{
	UExplorationSaveGameSubsystem* SaveSys = GetSaveSys();
	UExplorationSaveGame* Save = SaveSys ? SaveSys->GetSave() : nullptr;
	if (!Save) return;

	const double Now = NowReal();

	TArray<FGuid> ToActivate;
	for (const auto& KV : Save->RespawnAvailableAtReal)
	{
		if (Now >= KV.Value)
			ToActivate.Add(KV.Key);
	}

	for (const FGuid& ObjectId : ToActivate)
	{
		Save->RespawnAvailableAtReal.Remove(ObjectId);
		SaveSys->MarkDirty();

		TWeakObjectPtr<AExplorationObjectActor> Obj;
		if (TryGetObjectActor(ObjectId, Obj) && Obj.IsValid())
		{
			Obj->SetExplorationActive(true);
		}
	}
}

FExplorationOp UExplorationSubsystem::CheckLock(const UExplorationObjectDataAsset& Data) const
{
	const FExplorationLockCondition& L = Data.LockCondition;
	if (L.LockType == EExplorationLockType::None)
		return FExplorationOp::Ok();

	if (L.LockType == EExplorationLockType::KeyItem)
	{
		if (L.RequiredItemId.IsNone())
			return FExplorationOp::Fail("Reject.Lock.InvalidKeyItem");

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
	URewardServiceSubsystem* RewardService = GetRewardService();
	if (!RewardService) return FExplorationOp::Fail("Reject.NoRewardService");

	FRewardGrantRequest Req;
	Req.SourceObjectId = Data.ObjectId;
	Req.SourceTag = "Explore.RewardGranted";
	Req.TriggerType = TriggerType;
	Req.bOneTimeContext = (Data.AcquisitionPolicy == EExplorationAcquisitionPolicy::OneTime);
	Req.Table = Data.RewardTable;
	Req.DirectEntries = Data.RewardEntries;
	Req.Instigator = Instigator;

	TArray<FGrantedReward> Granted;
	const FExplorationOp GrantOp = RewardService->GrantRewards(Req, Granted);

	OnRewardsGranted.Broadcast(Data.ObjectId, Granted);

	// ExtensionFlag: “상점 오픈/문 열림” 같은 확장 플래그
	if (!Data.ExtensionFlag.IsNone())
	{
		if (UExplorationProgressSubsystem* P = GetProgressSys())
		{
			P->SetFlag(Data.ExtensionFlag, true);
			if (UExplorationSaveGameSubsystem* SaveSys = GetSaveSys())
				P->FlushToSave(SaveSys);
		}
	}

	// Save: OneTime/Respawn
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

	if (GetObjectState(ObjectId) != EExplorationObjectState::Active)
	{
		OnExplorationObjectCompleted.Broadcast(ObjectId, EExplorationOpResult::Rejected, "Reject.NotActive");
		return FExplorationOp::Fail("Reject.NotActive");
	}

	const FExplorationOp Lock = CheckLock(*Data);
	if (!Lock.bOk)
	{
		OnExplorationObjectCompleted.Broadcast(ObjectId, EExplorationOpResult::Rejected, Lock.ReasonTag);
		return Lock;
	}

	const FExplorationOp Done = CompleteObject(Interactor, *Data, EExplorationTriggerType::Interact);

	// OneTime/Respawn 둘 다 완료 후 비활성
	Obj->SetExplorationActive(false);

	OnExplorationObjectCompleted.Broadcast(
		ObjectId,
		Done.bOk ? EExplorationOpResult::Success : EExplorationOpResult::Rejected,
		Done.bOk ? "Success" : Done.ReasonTag
	);

	return Done;
}

FExplorationOp UExplorationSubsystem::NotifySolved(AActor* Instigator, const FGuid& ObjectId)
{
	TWeakObjectPtr<AExplorationObjectActor> Obj;
	if (!TryGetObjectActor(ObjectId, Obj)) return FExplorationOp::Fail("Reject.ObjectNotFound");
	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data) return FExplorationOp::Fail("Reject.NoData");
	if (GetObjectState(ObjectId) != EExplorationObjectState::Active) return FExplorationOp::Fail("Reject.NotActive");

	const FExplorationOp Done = CompleteObject(Instigator, *Data, EExplorationTriggerType::Solve);
	Obj->SetExplorationActive(false);

	OnExplorationObjectCompleted.Broadcast(
		ObjectId, Done.bOk ? EExplorationOpResult::Success : EExplorationOpResult::Rejected,
		Done.bOk ? "Success" : Done.ReasonTag);
	return Done;
}

FExplorationOp UExplorationSubsystem::NotifyDefeated(AActor* Instigator, const FGuid& ObjectId)
{
	TWeakObjectPtr<AExplorationObjectActor> Obj;
	if (!TryGetObjectActor(ObjectId, Obj)) return FExplorationOp::Fail("Reject.ObjectNotFound");
	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data) return FExplorationOp::Fail("Reject.NoData");
	if (GetObjectState(ObjectId) != EExplorationObjectState::Active) return FExplorationOp::Fail("Reject.NotActive");

	const FExplorationOp Done = CompleteObject(Instigator, *Data, EExplorationTriggerType::Defeat);
	Obj->SetExplorationActive(false);

	OnExplorationObjectCompleted.Broadcast(
		ObjectId, Done.bOk ? EExplorationOpResult::Success : EExplorationOpResult::Rejected,
		Done.bOk ? "Success" : Done.ReasonTag);
	return Done;
}

FExplorationOp UExplorationSubsystem::NotifyChallengeCompleted(AActor* Instigator, const FGuid& ObjectId)
{
	TWeakObjectPtr<AExplorationObjectActor> Obj;
	if (!TryGetObjectActor(ObjectId, Obj)) return FExplorationOp::Fail("Reject.ObjectNotFound");
	const UExplorationObjectDataAsset* Data = Obj->GetObjectData();
	if (!Data) return FExplorationOp::Fail("Reject.NoData");
	if (GetObjectState(ObjectId) != EExplorationObjectState::Active) return FExplorationOp::Fail("Reject.NotActive");

	const FExplorationOp Done = CompleteObject(Instigator, *Data, EExplorationTriggerType::Challenge);
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
		return FExplorationOp::Fail("Reject.DuplicateDiscovery");

	R.bDiscovered = true;
	R.DiscoveredAtRealTime = NowReal();
	SaveSys->MarkDirty();

	OnDiscoveryChanged.Broadcast(DiscoveryId, true);

	// 선택적 발견 보상(있을 때만)
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

			// Discovery는 ObjectId가 없으므로 Invalid GUID로 브로드캐스트
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
