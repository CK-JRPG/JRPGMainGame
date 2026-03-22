#include "Combat/Characters/CharacterRuntimeSubsystem.h"
#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

void UCharacterRuntimeSubsystem::SaveSnapshot(const FName& CharacterID, ACombatCharacterActor* Actor)
{
	if (!Actor) return;

	FCharacterResourceSnapshot Snap;

	if (Actor->HPComp)
	{
		Snap.HP    = Actor->HPComp->GetHP();
		Snap.MaxHP = Actor->HPComp->GetMaxHP();
	}
	if (Actor->APComp)
	{
		Snap.AP    = Actor->APComp->GetAP();
		Snap.MaxAP = Actor->APComp->GetMaxAP();
	}
	if (Actor->SPComp)
	{
		Snap.SP    = Actor->SPComp->GetSP();
		Snap.MaxSP = Actor->SPComp->GetMaxSP();
	}

	SnapshotMap.Add(CharacterID, Snap);

	UE_LOG(LogTemp, Log,
		TEXT("CharacterRuntimeSubsystem : 스냅샷 저장 [%s] HP=%.1f/%.1f AP=%d/%d SP=%d/%d"),
		*CharacterID.ToString(), Snap.HP, Snap.MaxHP, Snap.AP, Snap.MaxAP, Snap.SP, Snap.MaxSP);
}

void UCharacterRuntimeSubsystem::RestoreSnapshot(const FName& CharacterID, ACombatCharacterActor* Actor)
{
	if (!Actor) return;

	const FCharacterResourceSnapshot* Snap = SnapshotMap.Find(CharacterID);
	if (!Snap || !Snap->IsValid())
	{
		// 첫 인카운터 — 스냅샷 없음, DataAsset 기준값 그대로 사용
		return;
	}

	// HP: 풀 초기화 후 차이만큼 데미지 적용 (SetHP 직접 노출 없음)
	if (Actor->HPComp)
	{
		Actor->HPComp->InitializeHP(Snap->MaxHP, true);
		const float Damage = Snap->MaxHP - Snap->HP;
		if (Damage > 0.f)
			Actor->HPComp->ApplyDamage(Damage,nullptr, FName("Snapshot.Restore"));
	}

	// AP: 풀 초기화 후 차이만큼 소비 처리
	if (Actor->APComp)
	{
		Actor->APComp->InitializeAP(Snap->MaxAP, true);
		const int32 Consumed = Snap->MaxAP - Snap->AP;
		if (Consumed > 0)
			Actor->APComp->Consume(Consumed, FName("Snapshot.Restore"));
	}

	// SP: InitializeSP 두 번째 인자가 시작값을 직접 받음
	if (Actor->SPComp)
	{
		Actor->SPComp->InitializeSP(Snap->MaxSP, Snap->SP);
	}

	UE_LOG(LogTemp, Log,
		TEXT("CharacterRuntimeSubsystem : 스냅샷 복구 [%s] HP=%.1f/%.1f AP=%d/%d SP=%d/%d"),
		*CharacterID.ToString(), Snap->HP, Snap->MaxHP, Snap->AP, Snap->MaxAP, Snap->SP, Snap->MaxSP);
}

void UCharacterRuntimeSubsystem::InitializeSnapshotIfAbsent(const FName& CharacterID, float MaxHP, int32 MaxAP,
	int32 MaxSP)
{
	
	//이미 해당 캐릭터의 스냅샷이 있으면 생성 안함.
	if (SnapshotMap.Contains(CharacterID)) 
		return;

	FCharacterResourceSnapshot Snap;
	Snap.HP    = MaxHP;
	Snap.MaxHP = MaxHP;
	Snap.AP    = MaxAP;
	Snap.MaxAP = MaxAP;
	Snap.SP    = 0; 
	Snap.MaxSP = MaxSP;

	SnapshotMap.Add(CharacterID, Snap);

	UE_LOG(LogTemp, Log,
		TEXT("CharacterRuntimeSubsystem : 초기 스냅샷 생성 [%s] HP=%.1f AP=%d SP=%d"),
		*CharacterID.ToString(), MaxHP, MaxAP, MaxSP);
}

bool UCharacterRuntimeSubsystem::HasSnapshot(const FName& CharacterID) const
{
	const FCharacterResourceSnapshot* Snap = SnapshotMap.Find(CharacterID);
	return Snap && Snap->IsValid();
}

void UCharacterRuntimeSubsystem::ModifyHP(const FName& CharacterID, float Delta)
{
	FCharacterResourceSnapshot* Snap = SnapshotMap.Find(CharacterID);
	if (!Snap) return;

	Snap->HP = FMath::Clamp(Snap->HP + Delta, 0.f, Snap->MaxHP);

	UE_LOG(LogTemp, Log, TEXT("CharacterRuntimeSubsystem : HP 수정 [%s] %.1f → %.1f"),
		*CharacterID.ToString(), Snap->HP - Delta, Snap->HP);
}

void UCharacterRuntimeSubsystem::ModifyAP(const FName& CharacterID, int32 Delta)
{
	FCharacterResourceSnapshot* Snap = SnapshotMap.Find(CharacterID);
	if (!Snap) return;

	Snap->AP = FMath::Clamp(Snap->AP + Delta, 0, Snap->MaxAP);
}

void UCharacterRuntimeSubsystem::ModifySP(const FName& CharacterID, int32 Delta)
{
	FCharacterResourceSnapshot* Snap = SnapshotMap.Find(CharacterID);
	if (!Snap) return;

	Snap->SP = FMath::Clamp(Snap->SP + Delta, 0, Snap->MaxSP);
}

const FCharacterResourceSnapshot* UCharacterRuntimeSubsystem::GetSnapshot(const FName& CharacterID) const
{
	return SnapshotMap.Find(CharacterID);
}