#pragma once

#include "CoreMinimal.h"
#include "CharacterRuntimeStateTypes.generated.h"

/**
 * Actor나 World 수명과 무관하게 유지되는 캐릭터 자원 상태입니다.
 * 배치 정보(위치/회전)는 전투 전환 계층이 별도로 소유합니다.
 */
USTRUCT()
struct FCharacterRuntimeState
{
	GENERATED_BODY()

	UPROPERTY() float HP = 0.f;
	UPROPERTY() float MaxHP = 100.f;
	UPROPERTY() int32 AP = 0;
	UPROPERTY() int32 MaxAP = 10;
	UPROPERTY() int32 SP = 0;
	UPROPERTY() int32 MaxSP = 100;

	bool IsValid() const
	{
		return FMath::IsFinite(HP)
			&& FMath::IsFinite(MaxHP)
			&& MaxHP >= 1.f
			&& MaxAP >= 0
			&& MaxSP >= 0
			&& HP >= 0.f && HP <= MaxHP
			&& AP >= 0 && AP <= MaxAP
			&& SP >= 0 && SP <= MaxSP;
	}

	void Normalize()
	{
		MaxHP = FMath::Max(1.f, MaxHP);
		MaxAP = FMath::Max(0, MaxAP);
		MaxSP = FMath::Max(0, MaxSP);
		HP = FMath::Clamp(HP, 0.f, MaxHP);
		AP = FMath::Clamp(AP, 0, MaxAP);
		SP = FMath::Clamp(SP, 0, MaxSP);
	}
};

// 기존 UI/호출부의 타입 이름은 유지하되, 실제 데이터는 순수 Runtime State를 사용합니다.
using FCharacterResourceSnapshot = FCharacterRuntimeState;
