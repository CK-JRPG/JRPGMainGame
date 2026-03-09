#include"Combat/Skills/JRPGSkillComponent.h"

#include"Combat/Skills/JRPGSkillDataAsset.h"
#include"Combat/Skills/SkillExecutor.h"

#include"Combat/Tactical/TacticalTypes.h"
#include"Combat/Infrastructure/CombatTacticalModeSubsystem.h"
#include"Combat/Stats/CombatAPComponent.h"
#include"Combat/Stats/CombatHPComponent.h"
#include"Combat/Status/StatusComponent.h"

UJRPGSkillComponent::UJRPGSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

const UJRPGSkillDataAsset* UJRPGSkillComponent::GetSkillAsset(FName SkillId) const
{
	return FindSkill(SkillId);
}

void UJRPGSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	LastRealTime = FPlatformTime::Seconds();
}

void UJRPGSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CooldownRemaining.Reset();
	GlobalCooldownRemaining = 0.f;
	Super::EndPlay(EndPlayReason);
}

const UJRPGSkillDataAsset* UJRPGSkillComponent::FindSkill(FName SkillId) const
{
	if (const TObjectPtr<UJRPGSkillDataAsset>*Found =SkillDB.Find(SkillId))
		return Found->Get();
	return nullptr;
}

bool UJRPGSkillComponent::IsOnCooldown(FName SkillId)const
{
	const float *Rem = CooldownRemaining.Find(SkillId);
	return Rem && (*Rem>0.f);
}

float UJRPGSkillComponent::GetCooldownRemaining(FName SkillId)const
{
	if (const float *Rem = CooldownRemaining.Find(SkillId))
		return *Rem;
	return 0.f;
}

void UJRPGSkillComponent::StartCooldown(FName SkillId,float CooldownSec)
{
	if (CooldownSec <= 0.f) return;
	CooldownRemaining.Add(SkillId,CooldownSec);
}

void UJRPGSkillComponent::StartGlobalCooldown(float Sec)
{
	GlobalCooldownRemaining = FMath::Max(GlobalCooldownRemaining,Sec);
}

FJRPGSkillResult UJRPGSkillComponent::RequestUseSkill(const FJRPGSkillRequest &Req)
{
	FJRPGSkillResult Result;
	Result.SkillId = Req.SkillId;

	const UJRPGSkillDataAsset *Skill = FindSkill(Req.SkillId);
	if (!Skill)
	{
		Result.Op = FJRPGOpResult::Fail(EJRPGResultCode::NotFound, FJRPGReason::Make("Skill.NotFound"));
		OnSkillExecuted.Broadcast(Req,Result);
		return Result;
	}

	Result = FSkillExecutor::Execute(*this, *Skill,Req);

	if (Result.Op.bOk && Result.bExecuted)
	{
		if (!Req.bIgnoreCooldown)
		{
			StartCooldown(Skill->SkillId,Skill->Cooldown.CooldownSec);
		}
		if (!Req.bIgnoreGlobalCooldown)
		{
			StartGlobalCooldown(Skill->Cooldown.GlobalCooldownSec);
		}
	}

	OnSkillExecuted.Broadcast(Req,Result);
	return Result;
}

void UJRPGSkillComponent::TickCooldowns(float RealDelta)
{
	TArray<FName> ToRemove;
	for (auto&It :CooldownRemaining)
	{
		It.Value -= RealDelta;
		if (It.Value <= 0.f)
			ToRemove.Add(It.Key);
	}
	for (const FName&K : ToRemove)
		CooldownRemaining.Remove(K);

	if (GlobalCooldownRemaining > 0.f)
		GlobalCooldownRemaining = FMath::Max(0.f,GlobalCooldownRemaining - RealDelta);
}

bool UJRPGSkillComponent::CanUseReservedSkill(FName SkillId, FJRPGReason &OutReason)const
{
	// 문서: 예약 실행 시 CanUse(AP/쿨) 검사 :contentReference[oaicite:36]{index=36}
	if (SkillId.IsNone())
	{
		OutReason = FJRPGReason::Make("Tactical.BadSkillId");
		return false;
	}

	const UJRPGSkillDataAsset *Skill = FindSkill(SkillId);
	if (!Skill)
	{
		OutReason = FJRPGReason::Make("Tactical.SkillNotFound");
		return false;
	}

	// caster dead
	if (AActor* Owner =GetOwner())
	{
		if (UCombatHPComponent* HP = Owner->FindComponentByClass<UCombatHPComponent>())
		{
			if (HP->IsDead())
			{
				OutReason = FJRPGReason::Make("Tactical.CasterDead");
				return false;
			}
		}

		// CC면 실행 불가(스킬 문서의 기본 규칙과 일치)
		if (UStatusComponent* Status = Owner->FindComponentByClass<UStatusComponent>())
		{
			if (Status->IsCrowdControlled())
			{
				OutReason = FJRPGReason::Make("Tactical.CasterCC");
				return false;
			}
		}

		// cooldown/gcd
		if (IsOnCooldown(SkillId))
		{
			OutReason = FJRPGReason::Make("Tactical.OnCooldown");
			return false;
		}
		if (IsOnGlobalCooldown())
		{
			OutReason = FJRPGReason::Make("Tactical.OnGCD");
			return false;
		}

		// AP
		if (Skill->Cost.APCost>0)
		{
			if (UCombatAPComponent* AP = Owner->FindComponentByClass<UCombatAPComponent>())
			{
				if (!AP->CanSpend(Skill->Cost.APCost))
				{
					OutReason = FJRPGReason::Make("Tactical.NotEnoughAP");
					return false;
				}
			}
			else
			{
				OutReason = FJRPGReason::Make("Tactical.NoAPComponent");
				return false;
			}
		}
	}

	return true;
}

void UJRPGSkillComponent::TickTacticalReservation(float RealDelta)
{
	UWorld *W = GetWorld();
	if (!W) 
		return;

	UCombatTacticalModeSubsystem *Tactical = W->GetSubsystem<UCombatTacticalModeSubsystem>();
	if (!Tactical) 
		return;

	FTacticalReservation Res;
	if (!Tactical->GetReservation(GetOwner(),Res))
		return;

	// 타겟 무효(사망/삭제)면 기록 후 예약 해제 :contentReference[oaicite:37]{index=37}
	if (Res.Target.Kind == ETacticalTargetKind::Actor)
	{
		AActor* Tgt = Res.Target.TargetActor.Get();
		if (!Tgt || UCombatTacticalModeSubsystem::IsReservationTargetInvalid(Tgt))
		{
			Tactical->ClearReservation(GetOwner(),"Tactical.TargetInvalid");
			return;
		}
	}

	// CanUse(AP/쿨) 검사 후 가능한 순간 자동 실행 :contentReference[oaicite:38]{index=38}
	FJRPGReason Reason;
	const bool bCanUse = CanUseReservedSkill(Res.SkillId,Reason);

	// UI용 Ready 플래그 갱신(문서 Flags: Queued/Ready) :contentReference[oaicite:39]{index=39}
	if (bCanUse)
	{
		Tactical->SetReservationFlags(GetOwner(), (ETacticalReservationFlags)((uint8)ETacticalReservationFlags::Queued| (uint8)ETacticalReservationFlags::Ready));
	}
	else
	{
		Tactical->SetReservationFlags(GetOwner(), ETacticalReservationFlags::Queued);
		return;
	}

	// 실행 시도
	FJRPGSkillRequest Req;
	Req.SkillId = Res.SkillId;
	Req.Instigator = GetOwner();
	Req.SourceTag = "Tactical";
	Req.bFromTacticalReservation = true;// SP 전술 보너스 플래그(문서 연동) :contentReference[oaicite:40]{index=40}

	if (Res.Target.Kind == ETacticalTargetKind::Actor)
		Req.PrimaryTarget = Res.Target.TargetActor.Get();
	
	else if (Res.Target.Kind == ETacticalTargetKind::Location)
		Req.TargetLocation = Res.Target.TargetLocation;

	FJRPGSkillResult R = RequestUseSkill(Req);

	if (R.Op.bOk && R.bExecuted)
	{
		// 성공하면 예약 자동 해제 :contentReference[oaicite:41]{index=41}
		Tactical->ClearReservation(GetOwner(),"Tactical.Consumed");
	}
	// 실패면 유지(쿨/AP 충족될 때까지 유지가 핵심) :contentReference[oaicite:42]{index=42}
}

void UJRPGSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime,TickType,ThisTickFunction);

	const double Now = FPlatformTime::Seconds();
	const float RealDelta = (float)FMath::Max(0.0,Now - LastRealTime);
	LastRealTime = Now;

	if (RealDelta > 0.f)
	{
		TickCooldowns(RealDelta);

		// Tactical polling
		TacticalPollAccum += RealDelta;
		if (TacticalPollAccum >= TacticalPollIntervalSec)
		{
			TacticalPollAccum = 0.f;
			TickTacticalReservation(RealDelta);
		}
	}
}

bool UJRPGSkillComponent::HasSkill(FName SkillId) const
{
	return FindSkill(SkillId) != nullptr;
}

void UJRPGSkillComponent::GetOwnedSkillIds(TArray<FName>& OutSkillIds) const
{
	OutSkillIds.Reset();
	SkillDB.GetKeys(OutSkillIds);
}

bool UJRPGSkillComponent::CanUseSkill(FName SkillId) const
{
	FJRPGReason Reason;
	return CanUseReservedSkill(SkillId, Reason);
}

void UJRPGSkillComponent::RequestBasicAttack(AActor* Target)
{
	FName ChosenSkillId = NAME_None;
	for (const TPair<FName, TObjectPtr<UJRPGSkillDataAsset>>& It : SkillDB)
	{
		if (It.Key.ToString().Contains(TEXT("Basic")))
		{
			ChosenSkillId = It.Key;
			break;
		}
		if (ChosenSkillId.IsNone())
		{
			ChosenSkillId = It.Key;
		}
	}

	if (ChosenSkillId.IsNone())
	{
		return;
	}

	FJRPGSkillRequest Req;
	Req.SkillId = ChosenSkillId;
	if (Target) Req.AdditionalTargets.Add(Target);
	RequestUseSkill(Req);
}

void UJRPGSkillComponent::RequestUseSkillByAI(FName SkillId, AActor* Target)
{
	FJRPGSkillRequest Req;
	Req.SkillId = SkillId;
	if (Target) Req.AdditionalTargets.Add(Target);
	RequestUseSkill(Req);
}
