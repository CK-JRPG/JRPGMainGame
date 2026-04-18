#include "Combat/Infrastructure/CombatTimeSubsystem.h"

#include "Kismet/GameplayStatics.h"

void UCombatTimeSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);

	Entries.Reset();
	NextHandle = 1;

	AppliedScale = 1.0f;
	bBlending = false;
	BlendDuration = 0.f;

	LastReal = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	EnsureBackToNormal();
}

void UCombatTimeSubsystem::Deinitialize()
{
	Entries.Reset();
	EnsureBackToNormal();
	Super::Deinitialize();
}

void UCombatTimeSubsystem::EnsureBackToNormal()
{
	ApplyScale(1.0f);
}

FCombatTimeResult UCombatTimeSubsystem::RequestTimeMode(const FCombatTimeRequest&Req)
{
	FCombatTimeResult R;

	if (!GetWorld())
	{
		R.Op = FJRPGOpResult::Fail(EJRPGResultCode::Rejected, FJRPGReason::Make("Time.NoWorld"));
		return R;
	}

	// Basic validation
	if (Req.OwnerTag.IsNone())
	{
		R.Op = FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Time.NoOwnerTag"));
		return R;
	}

	const float Scale = FMath::Clamp(Req.TimeScale,0.01f,1.0f);
	const float Dur = FMath::Max(0.01f,Req.DurationRealSec);

	FEntry E;
	E.Handle.Value =NextHandle++;
	E.Req =Req;
		E.Req.TimeScale =Scale;
	E.Req.DurationRealSec =Dur;

	const double Now =GetWorld()->GetRealTimeSeconds();
	E.StartReal =Now;
	E.ExpireReal =Now+Dur;

	Entries.Add(E);

	R.Handle =E.Handle;
	R.Op = FJRPGOpResult::Ok();
	return R;
}

FJRPGOpResult UCombatTimeSubsystem::ReleaseTimeMode(FCombatTimeHandle Handle,FName/*ReasonTag*/)
{
	if (!Handle.IsValid())
	return FJRPGOpResult::Fail(EJRPGResultCode::Invalid, FJRPGReason::Make("Time.InvalidHandle"));

	const int32 Before =Entries.Num();
	Entries.RemoveAll([&](const FEntry &E) {return E.Handle.Value == Handle.Value; });

	if (Entries.Num()==Before)
	return FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Time.HandleNotFound"));

	return FJRPGOpResult::Ok();
}

void UCombatTimeSubsystem::RemoveExpired(double NowReal)
{
	Entries.RemoveAll([&](const FEntry &E) {return NowReal>=E.ExpireReal; });
}

const UCombatTimeSubsystem::FEntry* UCombatTimeSubsystem::PickWinningEntry()const
{
	if (Entries.Num()==0) return nullptr;

	const FEntry *Best = nullptr;
	for (const FEntry&E :Entries)
	{
		if (!Best) {Best = &E;continue; }
		
		if ((int32)E.Req.Priority> (int32)Best->Req.Priority)
			Best = &E;
		else if ((int32)E.Req.Priority== (int32)Best->Req.Priority)
		{
			// tie-break: newest wins
			if (E.StartReal>Best->StartReal)Best = &E;
		}
	}
	return Best;
}

float UCombatTimeSubsystem::DesiredScaleFromEntry(const FEntry*E)const
{
	if (!E) return 1.0f;

	switch (E->Req.Mode)
	{
	case ECombatTimeMode::Frozen:
		// UE SetGlobalTimeDilation은 0이 되면 위험해서 최소값으로 고정
		return FMath::Clamp(E->Req.TimeScale,0.001f,1.0f);

	case ECombatTimeMode::Slow:
		return FMath::Clamp(E->Req.TimeScale,0.01f,1.0f);

	case ECombatTimeMode::Normal:
	default:
		return 1.0f;
	}
}

void UCombatTimeSubsystem::StartBlend(float From, float To, float DurationSec, double NowReal)
{
	bBlending = true;
	BlendStartReal = NowReal;
	BlendDuration = FMath::Max(0.0f,DurationSec);
	BlendFrom = From;
	BlendTo = To;

	// duration이 0이면 즉시 적용
	if (BlendDuration <= 0.f)
	{
		bBlending = false;
		ApplyScale(BlendTo);
	}
}

void UCombatTimeSubsystem::ApplyScale(float NewScale)
{
	AppliedScale = FMath::Clamp(NewScale,0.01f,1.0f);
	if (UWorld*W =GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(W,AppliedScale);
	}
}

void UCombatTimeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld *W = GetWorld();
	if (!W) return;

	const double Now = W->GetRealTimeSeconds();
	RemoveExpired(Now);

	const FEntry *Winner = PickWinningEntry();
	const float Desired = DesiredScaleFromEntry(Winner);

	// desired가 바뀌었으면 새 블렌드
	if (!FMath::IsNearlyEqual(Desired,BlendTo,0.0001f) && !FMath::IsNearlyEqual(Desired,AppliedScale,0.0001f))
	{
		const float From = AppliedScale;
		const float To = Desired;

		// BlendIn/Out 선택: 1->slow는 BlendIn, slow->1은 BlendOut
		float Dur = 0.f;
		
		if (Winner && To < 1.0f)
			Dur = Winner -> Req.BlendInSec;
		
		else if (Winner && To >= 1.0f)
			Dur = Winner->Req.BlendOutSec;
		
		else
			Dur = 0.10f;

		StartBlend(From, To,Dur, Now);
	}

	// blend 진행
	if (bBlending)
	{
		const double Elapsed =Now-BlendStartReal;
		const float Alpha = (BlendDuration <= 0.f) ? 1.f : (float)FMath::Clamp(Elapsed/ (double)BlendDuration,0.0,1.0);
		const float Cur = FMath::Lerp(BlendFrom,BlendTo,Alpha);
		ApplyScale(Cur);

		if (Alpha>=1.0f)
		{
			bBlending =false;
		}
	}
	else
	{
		// 즉시 스냅(필요 시)
		if (!FMath::IsNearlyEqual(AppliedScale,Desired,0.0001f))
		{
			ApplyScale(Desired);
		}
	}

	// 아무 요청도 없으면 정상 복귀
	if (Entries.Num() == 0 && !FMath::IsNearlyEqual(AppliedScale,1.0f,0.0001f))
	{
		StartBlend(AppliedScale,1.0f,0.10f,Now);
	}
}