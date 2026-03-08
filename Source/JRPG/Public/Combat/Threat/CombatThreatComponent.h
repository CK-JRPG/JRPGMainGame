#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPG/Public/Combat/Threat/ThreatTypes.h"
#include "CombatThreatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnThreatTableChanged, AActor* /*Owner*/);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPG_API UCombatThreatComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UCombatThreatComponent();

	UPROPERTY(EditAnywhere) float ThreatDecayPerSec = 0.f;// 0이면 decay 없음
	FOnThreatTableChanged OnThreatTableChanged;

	void AddThreat(AActor *Source,float Amount,FName ReasonTag);
	float GetThreat(AActor *Source) const;
	AActor* GetTopThreatSource() const;

	void ClearAll();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
	UPROPERTY() TArray<FThreatEntry> Table;

	int32 FindIndex(AActor *Source) const;
	void Compact();
};