#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetGuideLineComponent.generated.h"

class UCombatCharacterComponent;
class UHPComponent;
class USkeletalMeshComponent;


UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UTargetGuideLineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetGuideLineComponent();

	UFUNCTION(BlueprintCallable, Category="Combat|Aggro Guide")
	void SetAggroTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category="Combat|Aggro Guide")
	void ClearAggroTarget();

	UFUNCTION(BlueprintPure, Category="Combat|Aggro Guide")
	AActor* GetAggroTarget() const { return AggroTarget.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// LineBatch 방식으로 어그로 라인을 그릴지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	bool bUseLineBatchGuide = true;

	// 라인 시작 위치로 사용할 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	FName StartSocketName = TEXT("AttackOrigin");

	// 시작 소켓이 없을 때 사용할 소유자 측 높이 보정값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float OwnerFallbackHeightOffset = 90.f;

	// 타겟 위치 계산 시 적용할 높이 보정값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float TargetHeightOffset = 80.f;

	// 거리 비례로 라인 곡률 높이를 얼마나 올릴지 결정하는 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float ArcHeightByDistance = 0.35f;

	// 라인 곡률의 최소 높이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float MinArcHeight = 120.f;

	// 라인 곡률의 최대 높이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float MaxArcHeight = 450.f;

	// 화면에 그려지는 라인의 두께
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float LineWidth = 7.f;

	// 라인의 전체 투명도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float Alpha = 0.85f;

	// 곡선을 몇 개의 직선으로 나눠 그릴지 결정하는 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	int32 LineSegmentCount = 8;

	// 각 라인 세그먼트가 유지되는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	float LineBatchLifetime = 0.05f;

	// 탱커 역할 타겟일 때 사용할 라인 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	FLinearColor DefenderTargetColor = FLinearColor(0.08f, 0.48f, 1.0f, 1.0f);

	// 탱커가 아닌 다른 역할 타겟일 때 사용할 라인 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Aggro Guide")
	FLinearColor NonDefenderTargetColor = FLinearColor(1.0f, 0.04f, 0.025f, 1.0f);

private:
	UPROPERTY(Transient) TWeakObjectPtr<AActor> AggroTarget;

	// ===== 캐싱 =====
	UPROPERTY(Transient) TWeakObjectPtr<UCombatCharacterComponent> OwnerCharacterComp;
	UPROPERTY(Transient) TWeakObjectPtr<UHPComponent> OwnerHPComp;
	UPROPERTY(Transient) TWeakObjectPtr<USkeletalMeshComponent> OwnerMeshComp;
	UPROPERTY(Transient) TWeakObjectPtr<UCombatCharacterComponent> TargetCharacterComp;
	UPROPERTY(Transient) TWeakObjectPtr<UHPComponent> TargetHPComp;
	UPROPERTY(Transient) TWeakObjectPtr<USkeletalMeshComponent> TargetMeshComp;

	void CacheTargetData(AActor* Target);
	void UpdateGuideLine();
	void DrawLineBatchGuide(const FVector& Start, const FVector& ControlPoint, const FVector& End, const FLinearColor& GuideColor) const;
	bool CanShowGuideTo(AActor* Target) const;
	bool IsActorDead(const AActor* Actor) const;
	FVector ResolveOwnerGuideLocation() const;
	FVector ResolveTargetGuideLocation(const AActor* Target) const;
	FLinearColor ResolveGuideColor(const AActor* Target) const;
};

