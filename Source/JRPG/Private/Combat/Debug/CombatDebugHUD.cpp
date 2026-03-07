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
				TEXT("[Battle] Active=%s State=%d Round=%d TurnIndex=%d Current=%s Paused=%s"),
					Battle->IsBattleActive() ?TEXT("true") :TEXT("false"),
					(int32)S.FlowState,
					S.Round,
					S.TurnIndex,
					S.CurrentTurnActor.IsValid() ? *S.CurrentTurnActor->GetName() : TEXT("-"),
					Battle->IsFlowPaused() ?TEXT("true") :TEXT("false"));
			Canvas->DrawShadowedString(X, Y, *BattleLine, Font, FLinearColor::White);
			Y += LineHeight;
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
			const FSynergyPointState& S = SP->GetState();
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