#include "Combat/Debug/CombatDebugHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

#include "Combat/Debug/CombatDebugSubsystem.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Tactical/TacticalModeSubsystem.h"
#include "Combat/Chain/ChainAttackSubsystem.h"
#include "Combat/SP/SynergyPointSubsystem.h"

static FColor ToFColorSafe(const FLinearColor& C)
{
	return C.ToFColor(true);
}

void ACombatDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !GetWorld() || !GEngine) return;

	UCombatDebugSubsystem* Debug = GetWorld()->GetSubsystem<UCombatDebugSubsystem>();
	if (!Debug || !Debug->IsOverlayEnabled()) return;

	UFont* Font = GEngine->GetSmallFont();
	if (!Font) return;

	float X = LeftX;
	float Y = TopY;

	if (bShowSummary)
	{
		if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
		{
			const FBattleSessionSnapshot& S = Battle->GetSnapshot();
			const FString BattleLine = FString::Printf(
				TEXT("[Battle] Active=%s Phase=%d AliveP=%d AliveE=%d ActiveActions=%d Exclusive=%s(%s)"),
					Battle->IsBattleActive() ? TEXT("true") : TEXT("false"),
					(int32)S.Phase,
					S.AlivePlayers,
					S.AliveEnemies,
					S.ActivePresentedActionCount,
					S.bExclusiveMode ? TEXT("true") : TEXT("false"),
					*S.ExclusiveModeTag.ToString());

			Canvas->DrawShadowedString(X, Y, *BattleLine, Font, FLinearColor::White);
			Y += LineHeight;

			TArray<FBattleActorRuntimeState> RuntimeStates;
			Battle->GetParticipantRuntimeStates(RuntimeStates);

			for (const FBattleActorRuntimeState& R : RuntimeStates)
			{
				const FString ActorName = R.Actor.IsValid() ? R.Actor->GetName() : TEXT("-");
				const FString RuntimeLine = FString::Printf(
					TEXT("  [%s] Team=%d Alive=%s Locked=%s Reason=%s Presented=%s Recovery=%.2f"),
						*ActorName,
						(int32)R.Team,
						R.bAlive ?TEXT("true") :TEXT("false"),
						R.bActionLocked ?TEXT("true") :TEXT("false"),
						*R.ActionLockReason.ToString(),
						R.bPresentedActionActive ?TEXT("true") :TEXT("false"),
						R.RemainingRecoverySec);

				const FLinearColor LineColor =
					!R.bAlive ? FLinearColor::Red :
					(R.bPresentedActionActive ? FLinearColor(0.9f, 0.8f, 0.3f) :
					(R.bActionLocked ? FLinearColor(1.f, 0.7f, 0.3f) : FLinearColor(0.7f, 1.f, 0.7f)));

				Canvas->DrawShadowedString(X, Y, *RuntimeLine, Font, LineColor);
				Y += LineHeight;
			}
		}

		if (UTacticalModeSubsystem* Tactical = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
		{
			const FString TacticalLine = FString::Printf(
				TEXT("[Tactical] Active=%s Operator=%s"),
					Tactical->IsActive() ? TEXT("true") : TEXT("false"),
					Tactical->GetSnapshot().OperatorActor.IsValid() ? *Tactical->GetSnapshot().OperatorActor->GetName() : TEXT("-"));
			Canvas->DrawShadowedString(X, Y, *TacticalLine, Font, FLinearColor(0.8f, 0.8f, 1.f));
			Y += LineHeight;
		}

		if (UChainAttackSubsystem* Chain = GetWorld()->GetSubsystem<UChainAttackSubsystem>())
		{
			const FChainAttackSnapshot& C = Chain->GetSnapshot();
			const FString ChainLine = FString::Printf(
				TEXT("[Chain] Active=%s Points=%d Step=%d Mult=%.2f Current=%s"),
					Chain->IsActive() ? TEXT("true") : TEXT("false"),
					C.RemainingChainPoints,
					C.StepIndex,
					C.CurrentDamageMultiplier,
					C.CurrentActor.IsValid() ? *C.CurrentActor->GetName() : TEXT("-"));
			Canvas->DrawShadowedString(X, Y, *ChainLine, Font, FLinearColor(1.f, 0.85f, 0.4f));
			Y += LineHeight;
		}

		if (USynergyPointSubsystem* SP = GetWorld()->GetSubsystem<USynergyPointSubsystem>())
		{
			const FJRPGSynergyPointState& S = SP->GetState();
			const FString SPLine = FString::Printf(
				TEXT("[SP] Current=%d Cap=%d Ready=%s"),
					S.CurrentSP,
					S.SPCap,
					S.bChainReady ? TEXT("true") : TEXT("false"));
			Canvas->DrawShadowedString(X, Y, *SPLine, Font, FLinearColor(0.5f, 1.f, 0.5f));
			Y+=LineHeight;
		}

		Y += 8.f;
		Canvas->DrawShadowedString(X, Y, TEXT("---- Recent Combat Logs ----"), Font, FLinearColor::Yellow);
		Y += LineHeight;
	}

	TArray<FCombatDebugEntry> Recent;
	Debug->GetRecentEntries(Debug->OverlayMaxLines, Recent);

	for (const FCombatDebugEntry& E :Recent)
	{
		const FString Line = FString::Printf(
			TEXT("[%.2f][%s][%s] %s | I=%s T=%s"),
				E.WorldTime,
				*LexToString((int32)E.Category),
				*E.Tag.ToString(),
				*E.Message,
				*E.InstigatorName,
				*E.TargetName);

		Canvas->DrawShadowedString(X, Y, *Line, Font, E.Color);
		Y += LineHeight;
	}
}