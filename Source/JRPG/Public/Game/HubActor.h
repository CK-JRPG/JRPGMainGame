#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "HubActor.generated.h"

class USphereComponent;

/*
 * 닥소 화톳불(?) 느낌의 허브 액터
 * 레벨에 배치하면 자동으로 HubSubsystem에 등록됨
 * TriggerSphere 범위 안에 플레이어가 진입하면 HubSubsystem에 포커스 허브로 등록
 * 실제 상호작용(E키) 입력은 JRPGPlayerController가 처리
 */
UCLASS()
class JRPG_API AHubActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AHubActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	
	// 이펙트
	UPROPERTY(VisibleAnywhere, Category="Hub|Niagara") TObjectPtr<UNiagaraComponent> NiagaraComp;
	
	// 플레이어 감지 트리거
	UPROPERTY(VisibleAnywhere, Category="Hub|Interact") TObjectPtr<USphereComponent> TriggerSphere;
	UPROPERTY(EditAnywhere, Category="Hub|Interact") float InteractRadius = 200.0f;
	
	
	// 오버랩 함수
	UFUNCTION() 
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
