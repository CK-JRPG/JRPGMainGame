// Source/JRPGCombat/Private/Combat/Progression/Leveling/TravelExpComponent.cpp
#include "Combat/Progression/Leveling/TravelExpComponent.h"
#include "Combat/Progression/Leveling/LevelingSubsystem.h"
#include "Combat/Progression/Leveling/ExpSettingsDataAsset.h"

#include "Engine/World.h"
#include "TimerManager.h"

UTravelExpComponent::UTravelExpComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTravelExpComponent::BeginPlay()
{
	Super::BeginPlay();

	LastLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(Timer, this, &UTravelExpComponent::Sample, SampleIntervalSec, true);
	}

	// Save에서 accumulator 이어받기
	if (ULevelingSubsystem* L = GetLeveling())
	{
		const FTravelExpAccumulator Acc = L->GetTravelAccumulator();
		AccumTimeSec = Acc.AccumTimeSec;
		AccumDistanceCm = Acc.AccumDistanceCm;
	}
}

void UTravelExpComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(Timer);
	}

	// 누산값 저장
	if (ULevelingSubsystem* L = GetLeveling())
	{
		FTravelExpAccumulator Acc = L->GetTravelAccumulator();
		Acc.AccumTimeSec = AccumTimeSec;
		Acc.AccumDistanceCm = AccumDistanceCm;
		Acc.LastSampleLocation = LastLoc;
		L->SetTravelAccumulator(Acc);
	}

	Super::EndPlay(EndPlayReason);
}

ULevelingSubsystem* UTravelExpComponent::GetLeveling() const
{
	return GetWorld() && GetWorld()->GetGameInstance()
		       ? GetWorld()->GetGameInstance()->GetSubsystem<ULevelingSubsystem>()
		       : nullptr;
}

const UExpSettingsDataAsset* UTravelExpComponent::GetSettings() const
{
	if (SettingsOverride) return SettingsOverride;
	if (ULevelingSubsystem* L = GetLeveling()) return L->ExpSettings;
	return nullptr;
}

int32 UTravelExpComponent::HashCell(const FVector& Loc, float Grid) const
{
	const float G = FMath::Max(1.f, Grid);
	const int32 X = (int32)FMath::FloorToInt(Loc.X / G);
	const int32 Y = (int32)FMath::FloorToInt(Loc.Y / G);
	const int32 Z = (int32)FMath::FloorToInt(Loc.Z / G);
	return HashCombineFast(HashCombineFast(GetTypeHash(X), GetTypeHash(Y)), GetTypeHash(Z));
}

void UTravelExpComponent::PushCell(int32 CellHash, int32 MaxSize)
{
	CellHistory.Add(CellHash);
	while (CellHistory.Num() > MaxSize)
	{
		CellHistory.RemoveAt(0);
	}
}

bool UTravelExpComponent::DetectBacktrackABA() const
{
	// A-B-A 패턴이면 “되돌아가기 반복”으로 간주 :contentReference[oaicite:45]{index=45}
	if (CellHistory.Num() < 3) return false;
	const int32 A = CellHistory[CellHistory.Num() - 3];
	const int32 B = CellHistory[CellHistory.Num() - 2];
	const int32 C = CellHistory[CellHistory.Num() - 1];
	return (A == C) && (A != B);
}

bool UTravelExpComponent::IsSameAreaFarm(const FVector& Loc, float RadiusCm) const
{
	// 같은 반경 내 반복 이동이면 지급 감소/정지 :contentReference[oaicite:46]{index=46}
	ULevelingSubsystem* L = GetLeveling();
	if (!L) return false;

	const FTravelExpAccumulator Acc = L->GetTravelAccumulator();
	const float R = FMath::Max(0.f, RadiusCm);
	return FVector::DistSquared(Loc, Acc.LastGrantLocation) <= (R * R);
}

void UTravelExpComponent::Sample()
{
	if (!GetOwner()) return;

	const UExpSettingsDataAsset* S = GetSettings();
	ULevelingSubsystem* L = GetLeveling();
	if (!S || !L) return;

	const FVector Cur = GetOwner()->GetActorLocation();
	const float Dist = FVector::Distance(Cur, LastLoc);

	AccumTimeSec += SampleIntervalSec;
	AccumDistanceCm += Dist;

	// 경로 해시 업데이트
	const int32 Cell = HashCell(Cur, S->GridQuantizeCm);
	PushCell(Cell, FMath::Max(3, S->PathHistorySize));

	// 지급 조건: time OR distance
	const bool bTimeReady = (S->TravelTickSec > 0.f) && (AccumTimeSec >= S->TravelTickSec);
	const bool bDistReady = (S->TravelDistanceCm > 0.f) && (AccumDistanceCm >= S->TravelDistanceCm);

	if (bTimeReady || bDistReady)
	{
		// Anti-exploit 판단
		float Mul = 1.0f;

		if (IsSameAreaFarm(Cur, S->SamePosRadiusCm))
		{
			// 같은 반경 파밍 → 정지
			Mul = 0.0f;
		}
		else if (DetectBacktrackABA())
		{
			// 되돌아가기 반복 → 감쇠
			Mul = FMath::Clamp(S->BacktrackPenaltyMul, 0.f, 1.f);
		}

		if (Mul <= 0.f)
		{
			// Reject.AntiExploit 예시(SSOT) :contentReference[oaicite:47]{index=47}
			AccumTimeSec = 0.f;
			AccumDistanceCm = 0.f;
			LastLoc = Cur;
			return;
		}

		// 실제 지급 (ContextGuid는 “이번 Travel grant 버킷”으로 생성)
		const int32 Base = FMath::Max(0, S->TravelBaseExp);
		const int32 FinalBase = (int32)FMath::FloorToInt((float)Base * Mul);

		const FGuid Context = FGuid::NewGuid();
		const FExpGrantOp R = L->GrantTravelExp(FinalBase, Context);

		// 지급 성공 시 LastGrantLocation 저장
		if (R.bOk)
		{
			FTravelExpAccumulator Acc = L->GetTravelAccumulator();
			Acc.LastGrantLocation = Cur;
			Acc.AccumTimeSec = 0.f;
			Acc.AccumDistanceCm = 0.f;
			Acc.LastSampleLocation = Cur;
			L->SetTravelAccumulator(Acc);
		}

		AccumTimeSec = 0.f;
		AccumDistanceCm = 0.f;
	}

	LastLoc = Cur;
}
