// Source/JRPGCombat/Private/Combat/AI/CombatAIContext.cpp

#include"Combat/AI/CombatAIContext.h"
#include"GameFramework/Pawn.h"
#include"Engine/World.h"

#include"Combat/Stats/HPComponent.h"
#include"Combat/Threat/ThreatComponent.h"
#include"Combat/Skills/SkillComponent.h"

void UCombatAIContext::Initialize(AActor*InOwner,EPartyRole InRole,UCombatAIPresetAsset*InPresetAsset)
{
	Owner = InOwner;
	OwnerPawn = Cast<APawn>(InOwner);
	OwnerController = OwnerPawn.IsValid() ? OwnerPawn->GetController() : nullptr;
	Role = InRole;
	PresetAsset = InPresetAsset;

	SkillComp = InOwner ? InOwner->FindComponentByClass<USkillComponent>() : nullptr;
	HPComp = InOwner ? InOwner->FindComponentByClass<UHPComponent>() : nullptr;
	ThreatComp = InOwner ? InOwner->FindComponentByClass<UThreatComponent>() : nullptr;
}

void UCombatAIContext::Refresh()
{
	if (!Owner.IsValid())
		return;

	RefreshSubsystemFlags();

	// Basic alive/hp
	if (HPComp.IsValid())
	{
		SelfHp01 = HPComp->GetHpRatio01();
		bSelfIsDead = HPComp->IsDead();
	}
	else
	{
		SelfHp01 = 1.f;
		bSelfIsDead = false;
	}

	RefreshPartySnapshot();
	RefreshTargetSnapshot();
	RefreshSP();
}

void UCombatAIContext::RefreshSubsystemFlags()
{
	// 여기서 BattleSessionSubsystem / TacticalModeSubsystem 등을 조회해 채움.
	// 지금은 “컴파일 가능한 형태”로 훅만 제공.
	bSessionActive = true;

	bInChainSequence = false;
	if (UWorld *World = Owner->GetWorld())
	{
		bool bChain = false;
		if (TryReadChainActiveFromWorld(World,bChain))
		{
			bInChainSequence = bChain;
		}
	}

	// Tactical은 전술 모드 Subsystem에서 가져옴 :contentReference[oaicite:16]{index=16}
	bInTactical = false;
}

void UCombatAIContext::RefreshPartySnapshot()
{
	// 실제 구현에서는 CombatParticipantRegistry에서 PartyMembers를 받음.
	// 여기서는 Owner 포함 파티 3인 정도를 전제로 외부에서 채우거나,
	// 프로젝트에서 Registry 연동 시 교체.
	bAnyAllyCritical = false;
	bAnyAllyHasCC = false;
	AllyCriticalTarget = nullptr;
	AllyCC_Target = nullptr;

	for (const TWeakObjectPtr<AActor> &Ally : PartyMembers)
	{
		if (!Ally.IsValid() || Ally.Get() == Owner.Get())
			continue;

		if (UHPComponent *AllyHP = Ally->FindComponentByClass<UHPComponent>())
		{
			const floatHp01 = AllyHP->GetHpRatio01();
			const floatCritThr = PresetAsset.IsValid() ? PresetAsset->Thresholds.PartyDangerHp01 : 0.30f;
			if (!AllyHP->IsDead() && Hp01<CritThr)
			{
				bAnyAllyCritical = true;
				AllyCriticalTarget = Ally;
				break;
			}
		}
	}

	// CC 탐지는 StatusComponent 쪽 GameplayTag로 처리(다음: 그로기/상태이상 구현에서 연결).
	// 지금은 훅만 남김.
}

void UCombatAIContext::RefreshTargetSnapshot()
{
	// PrimaryTarget은 문서: 기본은 세션 PrimaryTarget 공격 :contentReference[oaicite:17]{index=17}
	// 실제로는 BattleSessionSubsystem에서 가져오기.
	TargetGroggyPhase = EGroggyPhase::Normal;
	TargetBreakRatio01 = 0.f;
	if (PrimaryTarget.IsValid())
	{
		TryReadGroggyFromActor(PrimaryTarget.Get(),TargetGroggyPhase,TargetBreakRatio01);
	}
}

void UCombatAIContext::RefreshSP()
{
	CurrentSP = 0;
	SPCap = 0;
	bChainReady = false;
	SPSettings = FCombatSPSettingsView();

	if (UWorld *World = Owner->GetWorld())
	{
		TryReadSPFromWorld(World,CurrentSP,SPCap,bChainReady,SPSettings);
	}
}

bool UCombatAIContext::TryReadGroggyFromActor(AActor *Actor, EGroggyPhase &OutPhase, float &OutBreakRatio01)
{
	OutPhase = EGroggyPhase::Normal;
	OutBreakRatio01 = 0.f;

	if (!Actor)
		return false;

	// 컴포넌트가 ICombatGroggyProvider를 구현하면 그걸 읽는다.
	TArray<UActorComponent*> Comps;
	Actor->GetComponents(Comps);
	for (UActorComponent *C :Comps)
	{
		if (C && C->GetClass()->ImplementsInterface(UCombatGroggyProvider::StaticClass()))
		{
			ICombatGroggyProvider *Provider = Cast<ICombatGroggyProvider>(C);
			if (Provider)
			{
				OutPhase =Provider->GetGroggyPhase();
				OutBreakRatio01 = FMath::Clamp(Provider->GetBreakRatio01(),0.f,1.f);
				return true;
			}
		}
	}
	return false;
}

bool UCombatAIContext::TryReadSPFromWorld(UWorld *World, int32 &OutCurrent, int32 &OutCap, bool &OutReady, FCombatSPSettingsView &OutSettings)
{
	if (!World)
		return false;

	// 실제 구현: World Subsystem(예: SynergyPointSubsystem)이 ICombatSynergyPointProvider 구현하도록 만들면 됨.
	// 여기서는 훅만 제공.
	for (TObjectIterator<UObject> It; It; ++It)
	{
		UObject *Obj = *It;
		if (!Obj || Obj->GetWorld() != World)
			continue;

		if (Obj->GetClass()->ImplementsInterface(UCombatSynergyPointProvider::StaticClass()))
		{
			ICombatSynergyPointProvider *SP =Cast<ICombatSynergyPointProvider>(Obj);
			if (SP)
			{
				OutCurrent = SP->GetCurrentSP();
				OutCap = SP->GetSPCap();
				OutReady = SP->IsChainReady();
				OutSettings = SP->GetSettingsView();
				return true;
			}
		}
	}
	return false;
}

bool UCombatAIContext::TryReadChainActiveFromWorld(UWorld *World, bool &bOutChainActive)
{
	if (!World)
		return false;

	for (TObjectIterator<UObject> It; It; ++It)
	{
		UObject *Obj = *It;
		if (!Obj || Obj->GetWorld() != World)
			continue;

		if (Obj->GetClass()->ImplementsInterface(UCombatChainFlowProvider::StaticClass()))
		{
			ICombatChainFlowProvider *Chain =Cast<ICombatChainFlowProvider>(Obj);
			if (Chain)
			{
				bOutChainActive = Chain->IsChainSequenceActive();
				return true;
			}
		}
	}
	return false;
}