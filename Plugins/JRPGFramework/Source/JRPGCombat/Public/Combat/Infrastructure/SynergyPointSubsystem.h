#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JRPGCoreApiTypes.h"
#include "Combat/SP/SPTypes.h"
#include "SynergyPointSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnJRPGSPChanged, const FJRPGSPState&);

UCLASS()
class JRPGCOMBAT_API USynergyPointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	FOnJRPGSPChanged OnSPChanged;

	const FJRPGSPState& GetState() const { return State; }

	FJRPGOpResult ApplyGainEvent(const FJRPGSPGainEvent& Ev);
	FJRPGOpResult ConsumeForChain(int32 Cost, FName ReasonTag);

private:
	FJRPGSPState State;
};