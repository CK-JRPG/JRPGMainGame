#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "HubLocationActor.generated.h"

class UCombatTransitionSubsystem;

UCLASS()
class JRPG_API AHubLocationActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AHubLocationActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	
	// 이펙트
	UPROPERTY(VisibleAnywhere, Category="Niagara") TObjectPtr<UNiagaraComponent> NiagaraComp;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UBillboardComponent> EditorSprite;
#endif
};
