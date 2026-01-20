#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillComponent.generated.h"

class USkillDataAsset;
class UCombatSkill;
class UAPComponent;

USTRUCT()
struct FCooldownEntry
{
	GENERATED_BODY()
	UPROPERTY() float Remaining = 0.f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JRPG_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillComponent();

	UPROPERTY(EditAnywhere, Category="Skills") TArray<TObjectPtr<USkillDataAsset>> SkillAssets;

	void InitializeSkills();
	TArray<UCombatSkill*> GetSkills() const { return RuntimeSkills; }

	bool TryExecuteSkill(UCombatSkill* Skill, AActor* Target);

	bool IsOnCooldown(const UCombatSkill* Skill) const;
	float GetCooldownRemaining(const UCombatSkill* Skill) const;

	UCombatSkill* GetBasicAttack() const { return BasicAttack; }

	void ReserveSkill(UCombatSkill* Skill) { ReservedSkill = Skill; }
	void ClearReservedSkill() { ReservedSkill = nullptr; }
	bool HasReservedSkill() const { return ReservedSkill != nullptr; }
	UCombatSkill* GetReservedSkill() const { return ReservedSkill; }
	FName GetReservedSkillId() const;

	UCombatSkill* FindSkillById(FName SkillId) const;

	void ReduceAllCooldownsByPercent(float Percent);

	UCombatSkill* GetSkillBySlot(int32 Slot1Based) const; // 1~4, Basic 제외
	int32 GetSkillSlotCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TArray<TObjectPtr<UCombatSkill>> RuntimeSkills;
	TMap<TObjectPtr<UCombatSkill>, FCooldownEntry> Cooldowns;

	UPROPERTY() TObjectPtr<UCombatSkill> ReservedSkill = nullptr;
	UPROPERTY() TObjectPtr<UCombatSkill> BasicAttack = nullptr;

	UAPComponent* GetAP() const;
	void AddRuntimeSkill(UCombatSkill* Skill);
};
