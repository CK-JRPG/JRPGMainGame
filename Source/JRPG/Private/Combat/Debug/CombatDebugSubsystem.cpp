#include "Combat/Debug/CombatDebugSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"
#include "Combat/Chain/ChainAttackSubsystem.h"
#include "Combat/SP/SynergyPointSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatDebugStream, Log, All);

static FString DebugCategoryToString(ECombatDebugCategory C)
{
	switch (C)
	{
	case ECombatDebugCategory::System: return TEXT("System");
	case ECombatDebugCategory::Session: return TEXT("Session");
	case ECombatDebugCategory::Turn: return TEXT("Turn");
	case ECombatDebugCategory::Action: return TEXT("Action");
	case ECombatDebugCategory::Skill: return TEXT("Skill");
	case ECombatDebugCategory::Item: return TEXT("Item");
	case ECombatDebugCategory::Tactical: return TEXT("Tactical");
	case ECombatDebugCategory::Chain: return TEXT("Chain");
	case ECombatDebugCategory::SP: return TEXT("SP");
	case ECombatDebugCategory::Motion: return TEXT("Motion");
	case ECombatDebugCategory::Status: return TEXT("Status");
	case ECombatDebugCategory::Groggy: return TEXT("Groggy");
	case ECombatDebugCategory::Threat: return TEXT("Threat");
	case ECombatDebugCategory::Presentation: return TEXT("Presentation");
	case ECombatDebugCategory::AI: return TEXT("AI");
	default: return TEXT("Unknown");
	}
}

FString UCombatDebugSubsystem::ActorNameSafe(AActor* Actor) const
{
	return Actor ? Actor->GetName() : TEXT("-");
}

void UCombatDebugSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BindGlobalEvents();
	AddLog(ECombatDebugCategory::System, "Debug.Init", "CombatDebugSubsystem initialized", nullptr, nullptr, FLinearColor::Green);
}

void UCombatDebugSubsystem::BindGlobalEvents()
{
	if (!GetWorld()) return;

	if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
	{
		Battle->OnBattleStarted.AddUObject(this, &UCombatDebugSubsystem::HandleBattleStarted);
		Battle->OnBattleEnded.AddUObject(this, &UCombatDebugSubsystem::HandleBattleEnded);
		Battle->OnTurnStarted.AddUObject(this, &UCombatDebugSubsystem::HandleTurnStarted);
		Battle->OnTurnEnded.AddUObject(this, &UCombatDebugSubsystem::HandleTurnEnded);
		Battle->OnBattleStateChanged.AddUObject(this, &UCombatDebugSubsystem::HandleBattleStateChanged);
	}

	if (UBasicCombatSubsystem* Basic = GetWorld()->GetSubsystem<UBasicCombatSubsystem>())
	{
		Basic->OnBasicAttackResolved.AddUObject(this, &UCombatDebugSubsystem::HandleBasicAttackResolved);
		Basic->OnCombatantDefeated.AddUObject(this, &UCombatDebugSubsystem::HandleCombatantDefeated);
	}

	if (UTacticalModeSubsystem* Tactical = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
	{
		Tactical->OnTacticalModeEntered.AddUObject(this, &UCombatDebugSubsystem::HandleTacticalEntered);
		Tactical->OnTacticalModeExited.AddUObject(this, &UCombatDebugSubsystem::HandleTacticalExited);
		Tactical->OnTacticalReservationChanged.AddUObject(this, &UCombatDebugSubsystem::HandleTacticalReservationChanged);
	}

	if (UChainAttackSubsystem* Chain =GetWorld()->GetSubsystem<UChainAttackSubsystem>())
	{
		Chain->OnChainAttackStarted.AddUObject(this, &UCombatDebugSubsystem::HandleChainStarted);
		Chain->OnChainAttackStepResolved.AddUObject(this, &UCombatDebugSubsystem::HandleChainStepResolved);
		Chain->OnChainAttackEnded.AddUObject(this, &UCombatDebugSubsystem::HandleChainEnded);
	}

	if (USynergyPointSubsystem* SP = GetWorld()->GetSubsystem<USynergyPointSubsystem>())
	{
		SP->OnSynergyPointChanged.AddUObject(this, &UCombatDebugSubsystem::HandleSPChanged);
		SP->OnSynergyReadyChanged.AddUObject(this, &UCombatDebugSubsystem::HandleSPReadyChanged);
		SP->OnSynergyGainApplied.AddUObject(this, &UCombatDebugSubsystem::HandleSPGainApplied);
	}
}

void UCombatDebugSubsystem::AddLog(
	ECombatDebugCategory Category,
	FName Tag,
	const FString& Message,
	AActor* Instigator,
	AActor* Target,
	FLinearColor Color)
{
	FCombatDebugEntry E;
	E.RealTime = FPlatformTime::Seconds();
	E.WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	E.Category = Category;
	E.Tag = Tag;
	E.Message = Message;
	E.InstigatorName = ActorNameSafe(Instigator);
	E.TargetName = ActorNameSafe(Target);
	E.Color = Color;

	Entries.Add(E);
	if (Entries.Num() > MaxEntries)
	{
		const int32 Overflow = Entries.Num()-MaxEntries;
		Entries.RemoveAt(0,Overflow,false);
	}

	if (bEchoToOutputLog)
	{
		UE_LOG(
		LogCombatDebugStream,
		Log,
		TEXT("[%.2f][%s][%s] %s | Instigator=%s Target=%s"),
			E.WorldTime,
			*DebugCategoryToString(Category),
			*Tag.ToString(),
			*Message,
			*E.InstigatorName,
			*E.TargetName);
	}
}

void UCombatDebugSubsystem::ClearLogs()
{
	Entries.Reset();
	AddLog(ECombatDebugCategory::System, "Debug.Clear", "Combat debug logs cleared", nullptr, nullptr, FLinearColor::Yellow);
}

void UCombatDebugSubsystem::GetRecentEntries(int32 MaxCount, TArray<FCombatDebugEntry>& OutEntries) const
{
	OutEntries.Reset();

	if (MaxCount <= 0 || Entries.Num() <= 0)
	{
		return;
	}

	const int32 Start = FMath::Max(0, Entries.Num() - MaxCount);
	for (int32 i = Start; i < Entries.Num(); ++i)
	{
		OutEntries.Add(Entries[i]);
	}
}

void UCombatDebugSubsystem::HandleBattleStarted(const FBattleSessionSnapshot& Snapshot)
{
	AddLog(
		ECombatDebugCategory::Session,
		"Battle.Start",
		FString::Printf(TEXT("Battle started | Session=%s Round=%d"), *Snapshot.SessionId.ToString(), Snapshot.Round),
		nullptr, nullptr, FLinearColor(0.2f, 1.f, 0.2f));
}

void UCombatDebugSubsystem::HandleBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason)
{
	AddLog(
		ECombatDebugCategory::Session,
		"Battle.End",
		FString::Printf(TEXT("Battle ended | Session=%s Reason=%d"), *Snapshot.SessionId.ToString(), (int32)Reason),
		nullptr, nullptr, FLinearColor(1.f, 0.5f, 0.2f));
}

void UCombatDebugSubsystem::HandleTurnStarted(AActor* Actor,int32 Round)
{
	AddLog(
		ECombatDebugCategory::Turn,
		"Turn.Start",
		FString::Printf(TEXT("Turn started | Round=%d Actor=%s"), Round, *ActorNameSafe(Actor)),
		Actor, nullptr, FLinearColor::Cyan);
}

void UCombatDebugSubsystem::HandleTurnEnded(AActor* Actor, int32 Round)
{
	AddLog(
		ECombatDebugCategory::Turn,
		"Turn.End",
		FString::Printf(TEXT("Turn ended | Round=%d Actor=%s"), Round, *ActorNameSafe(Actor)),
		Actor, nullptr, FLinearColor(0.5f, 0.9f, 1.f));
}

void UCombatDebugSubsystem::HandleBattleStateChanged(EBattleFlowState NewState)
{
	AddLog(
		ECombatDebugCategory::Session,
		"Battle.State",
		FString::Printf(TEXT("FlowState changed -> %d"), (int32)NewState),
		nullptr, nullptr, FLinearColor::Silver);
}

void UCombatDebugSubsystem::HandleBasicAttackResolved(const FCombatActionResult& Result)
{
	AddLog(
		ECombatDebugCategory::Action,
		Result.ReasonTag.IsNone() ? "BasicAttack" : Result.ReasonTag,
		FString::Printf(
			TEXT("BasicAttack resolved | Damage=%.0f Crit=%s TargetDied=%s"),
			Result.Breakdown.FinalDamage,
			Result.Breakdown.bCritical ? TEXT("true") : TEXT("false"),
			Result.bTargetDied ? TEXT("true") : TEXT("false")),
		Result.Attacker.Get(),
		Result.Target.Get(),
		FLinearColor::White);
}

void UCombatDebugSubsystem::HandleCombatantDefeated(AActor* Victim, AActor* Killer)
{
	AddLog(
		ECombatDebugCategory::Action,
		"Defeat",
		FString::Printf(TEXT("%s defeated by %s"), *ActorNameSafe(Victim), *ActorNameSafe(Killer)),
		Killer,
		Victim,
		FLinearColor::Red);
}

void UCombatDebugSubsystem::HandleTacticalEntered(const FTacticalModeSnapshot& Snapshot)
{
	AddLog(
		ECombatDebugCategory::Tactical,
		"Tactical.Enter",
		FString::Printf(TEXT("Tactical mode entered | Operator=%s"), *ActorNameSafe(Snapshot.OperatorActor.Get())),
		Snapshot.OperatorActor.Get(),
		nullptr,
		FLinearColor(0.7f, 0.9f, 1.f));
}

void UCombatDebugSubsystem::HandleTacticalExited(const FTacticalModeSnapshot& Snapshot)
{
	AddLog(
		ECombatDebugCategory::Tactical,
		"Tactical.Exit",
		FString::Printf(TEXT("Tactical mode exited | Operator=%s"), *ActorNameSafe(Snapshot.OperatorActor.Get())),
		Snapshot.OperatorActor.Get(),
		nullptr,
		FLinearColor(0.7f,0.7f,1.f));
}

void UCombatDebugSubsystem::HandleTacticalReservationChanged(AActor* Actor, bool bHasReservation, FName SkillId)
{
	AddLog(
		ECombatDebugCategory::Tactical,
		"Tactical.Reservation",
		FString::Printf(TEXT("Reservation %s | Skill=%s"),bHasReservation ?TEXT("SET") :TEXT("CLEARED"), *SkillId.ToString()),
		Actor,
		nullptr,
		FLinearColor(0.8f, 0.8f, 1.f));
}

void UCombatDebugSubsystem::HandleChainStarted(const FChainAttackSnapshot& Snapshot)
{
	AddLog(
		ECombatDebugCategory::Chain,
		"Chain.Start",
		FString::Printf(TEXT("Chain started | Starter=%s Points=%d Mult=%.2f"), 
				*ActorNameSafe(Snapshot.ChainStarter.Get()),
				Snapshot.RemainingChainPoints,
				Snapshot.CurrentDamageMultiplier),
		Snapshot.ChainStarter.Get(),
		nullptr,
		FLinearColor(1.f, 0.85f, 0.2f));
}

void UCombatDebugSubsystem::HandleChainStepResolved(const FChainAttackSnapshot& Snapshot, AActor* ActingActor)
{
	AddLog(
		ECombatDebugCategory::Chain,
		"Chain.Step",
		FString::Printf(TEXT("Chain step resolved | Actor=%s Remaining=%d Mult=%.2f"),
			*ActorNameSafe(ActingActor),
			Snapshot.RemainingChainPoints,
			Snapshot.CurrentDamageMultiplier),
		ActingActor,
		nullptr,
		FLinearColor(1.f, 0.9f, 0.4f));
}

void UCombatDebugSubsystem::HandleChainEnded(const FChainAttackSnapshot& Snapshot)
{
	AddLog(
		ECombatDebugCategory::Chain,
		"Chain.End",
		FString::Printf(TEXT("Chain ended | Starter=%s"), *ActorNameSafe(Snapshot.ChainStarter.Get())),
		Snapshot.ChainStarter.Get(),
		nullptr,
		FLinearColor(1.f, 0.75f, 0.2f));
}

void UCombatDebugSubsystem::HandleSPChanged(int32 CurrentSP, int32 Delta, ESPEventType Type, FName ReasonTag)
{
	AddLog(
		ECombatDebugCategory::SP,
		ReasonTag.IsNone() ?"SP.Change" : ReasonTag,
		FString::Printf(TEXT("SP changed | Current=%d Delta=%d EventType=%d"), CurrentSP, Delta, (int32)Type),
		nullptr,nullptr,
		FLinearColor(0.4f,1.f,0.4f));
}

void UCombatDebugSubsystem::HandleSPReadyChanged(bool bReady)
{
	AddLog(
		ECombatDebugCategory::SP,
		"SP.Ready",
		FString::Printf(TEXT("ChainReady -> %s"), bReady ? TEXT("true") : TEXT("false")),
		nullptr,nullptr,
		bReady ? FLinearColor::Green : FLinearColor::Yellow);
}

void UCombatDebugSubsystem::HandleSPGainApplied(const FSPGainEvent& Event)
{
	AddLog(
		ECombatDebugCategory::SP,
		Event.ReasonTag.IsNone() ? "SP.Gain" : Event.ReasonTag,
		FString::Printf(TEXT("SP gain applied | Base=%d Role=%d Tactical=%d Final=%d RoleType=%d Tactical=%s"),
				Event.BaseAmount,
				Event.RoleBonusAmount,
				Event.TacticalBonusAmount,
				Event.FinalGrantedAmount,
				(int32)Event.Role,
				Event.bFromTacticalReservation ?TEXT("true") :TEXT("false")),
		Event.Instigator.Get(),
		Event.Target.Get(),
		FLinearColor(0.5f,1.f,0.5f));
}