#include "Combat/Status/StatusEffectComponent.h"

#include "Combat/Characters/Stats/CharacterCombatStatsComponent.h"
#include "Combat/Stats/HPComponent.h"

#include "Combat/Debug/CombatDebugSubsystem.h"
#include "Combat/Characters/Stats/CharacterCombatStatsComponent.h"

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick =true;
	PrimaryComponentTick.TickInterval =0.10f;
}

void UStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();
	
	
	Stats = GetOwner() ? GetOwner()->FindComponentByClass<UCharacterCombatStatsComponent>() : nullptr;
	HP = GetOwner() ? GetOwner()->FindComponentByClass<UHPComponent>() : nullptr;
}

int32 UStatusEffectComponent::FindIdx(FName EffectId)const
{
	for (int32 i=0; i < Active.Num(); ++i)
		if (Active[i].Def&&Active[i].Def->EffectId == EffectId)
			return i;
	return INDEX_NONE;
}

bool UStatusEffectComponent::HasStatus(FName EffectId) const
{
	return FindIdx(EffectId) != INDEX_NONE;
}

void UStatusEffectComponent::AddMods(FEffectActiveStatus &S)
{
	if (!Stats.IsValid()||!S.Def)return;

	if (!S.ModSource)
	{
		S.ModSource =NewObject<UStatusModSourceObject>(this);
		S.ModSource->EffectId =S.Def->EffectId;
	}

	for (const FCombatStatModifier &M0 : S.Def->StatMods)
	{
		FCombatStatModifier M = M0;
		M.Source = S.ModSource;
		
		if (M.Tag.IsNone()) 
			M.Tag = S.Def->EffectId;
		
		Stats->AddModifier(M); // 함수없음
	}
}

void UStatusEffectComponent::RemoveMods(FEffectActiveStatus &S)
{
	if (!Stats.IsValid() || !S.ModSource) return;
	Stats->RemoveModifiersBySource(S.ModSource); // 함수없음
}

bool UStatusEffectComponent::ApplyStatus(UStatusEffectDataAsset *Effect,AActor *Source,int32 AddStacks, FName)
{
	if (!Effect||!Effect->IsValidDef())return false;
	AddStacks = FMath::Max(1,AddStacks);

	const int32 Idx =FindIdx(Effect->EffectId);
	if (Idx == INDEX_NONE)
	{
		FEffectActiveStatus S;
		S.Def =Effect;
		S.Applier =Source;
		S.Stacks = FMath::Clamp(AddStacks,1, FMath::Max(1,Effect->MaxStacks));
		S.Remaining = FMath::Max(0.01f,Effect->DurationSec);
		S.NextTick =Effect->Periodic.PeriodSec;

		AddMods(S);

		Active.Add(S);
		OnStatusApplied.Broadcast(Effect->EffectId,S.Stacks);
		
		if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
		{
			Debug->AddLog(
				ECombatDebugCategory::Status,
				Effect->EffectId,
				FString::Printf(TEXT("Status applied | Stacks=%d"), AddStacks),
				Source,
				GetOwner(),
				FLinearColor(0.9f, 0.6f, 1.f));
		}
		
		return true;
	}

	// existing
	FEffectActiveStatus &S =Active[Idx];
	if (!S.Def) {S.Def = Effect; }

	switch (Effect->StackPolicy)
	{
	case EJRPGStatusStackPolicy::RefreshDuration:
		S.Remaining = FMath::Max(0.01f,Effect->DurationSec);
		break;
 		
	case EJRPGStatusStackPolicy::AddStacksClamp:
		S.Stacks = FMath::Clamp(S.Stacks + AddStacks, 1, FMath::Max(1, Effect->MaxStacks));
		S.Remaining = FMath::Max(0.01f, Effect->DurationSec);
		break;
 		
	case EJRPGStatusStackPolicy::IgnoreIfExists:
	default:
		break;
	}

	OnStatusApplied.Broadcast(Effect->EffectId, S.Stacks);
	
	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Status,
			Effect->EffectId,
			FString::Printf(TEXT("Status applied | Stacks=%d"), AddStacks),
			Source,
			GetOwner(),
			FLinearColor(0.9f, 0.6f, 1.f));
	}
	
	return true;
}

bool UStatusEffectComponent::RemoveStatus(FName EffectId, FName)
{
	const int32 Idx = FindIdx(EffectId);
	if (Idx == INDEX_NONE) 
		return false;

	FEffectActiveStatus S = Active[Idx];
	RemoveMods(S);

	Active.RemoveAt(Idx);
	OnStatusRemoved.Broadcast(EffectId);
	
	if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
	{
		Debug->AddLog(
			ECombatDebugCategory::Status,
			EffectId,
			TEXT("Status removed"),
			GetOwner(),
			GetOwner(),
			FLinearColor(0.8f,0.8f,1.f));
	}
	
	return true;
}

void UStatusEffectComponent::TickPeriodic(FEffectActiveStatus &S, float DeltaTime)
{
	if (!S.Def)
		return;
	const float P = S.Def->Periodic.PeriodSec;
	if (P <= 0.f)
		return;

	S.NextTick -= DeltaTime;
	while (S.NextTick <= 0.f)
	{
		S.NextTick += P;

		const float Dmg = S.Def->Periodic.DamagePerTick * (float)S.Stacks;
		const float Heal = S.Def->Periodic.HealPerTick * (float)S.Stacks;

		if (HP.IsValid())
		{
			if (Dmg>0.f)
				HP->ApplyDamage(Dmg, S.Applier.Get(), S.Def->EffectId);
			if (Heal>0.f)
				HP->Heal(Heal, S.Applier.Get(), S.Def->EffectId);
		}
	}
}

void UStatusEffectComponent::TickExpiry(float DeltaTime)
{
	for (int32 i = Active.Num() - 1; i >= 0; --i)
	{
		FEffectActiveStatus &S = Active[i];
		if (!S.Def)
		{
			Active.RemoveAt(i); 
			continue;
		}

		S.Remaining -= DeltaTime;
		if (S.Remaining <= 0.f)
		{
			FName Id = S.Def->EffectId;
			RemoveMods(S);
			Active.RemoveAt(i);
			OnStatusRemoved.Broadcast(Id);
			
			if (UCombatDebugSubsystem* Debug = GetWorld() ? GetWorld()->GetSubsystem<UCombatDebugSubsystem>() : nullptr)
			{
				Debug->AddLog(
					ECombatDebugCategory::Status,
					Id,
					TEXT("Status expired"),
					GetOwner(),
					GetOwner(),
					FLinearColor(0.7f, 0.7f, 1.f));
			}
		}
	}
}

void UStatusEffectComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	for (FEffectActiveStatus &S : Active)
		TickPeriodic(S,DeltaTime);
	
	TickExpiry(DeltaTime);
}