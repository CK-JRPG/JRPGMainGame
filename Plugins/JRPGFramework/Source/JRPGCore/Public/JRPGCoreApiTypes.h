
#pragma once

#include "CoreMinimal.h"
#include "JRPGCoreApiTypes.generated.h"

UENUM(BlueprintType)
enum class EJRPGResultCode : uint8
{
	Success UMETA(DisplayName="Success"),
	Rejected UMETA(DisplayName="Rejected"),
	Invalid UMETA(DisplayName="Invalid"),
	NotFound UMETA(DisplayName="NotFound"),
	Insufficient UMETA(DisplayName="Insufficient"),
	Conflict UMETA(DisplayName="Conflict"),
	InternalError UMETA(DisplayName="InternalError")
};

USTRUCT(BlueprintType)
struct JRPGCORE_API FJRPGReason
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Message;

	static FJRPGReason None()
	{
		return FJRPGReason{};
	}

	static FJRPGReason Make(FName InTag, const FText& InMessage = FText::GetEmpty())
	{
		FJRPGReason Reason;
		Reason.Tag = InTag;
		Reason.Message = InMessage;
		return Reason;
	}

	bool IsNone() const
	{
		return Tag.IsNone();
	}
};

USTRUCT(BlueprintType)
struct JRPGCORE_API FJRPGOpResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOk = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EJRPGResultCode Code = EJRPGResultCode::Success;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FJRPGReason Reason;

	static FJRPGOpResult Ok()
	{
		return FJRPGOpResult{};
	}

	static FJRPGOpResult Fail(EJRPGResultCode InCode, const FJRPGReason& InReason)
	{
		FJRPGOpResult Result;
		Result.bOk = false;
		Result.Code = InCode;
		Result.Reason = InReason;
		return Result;
	}
};

USTRUCT(BlueprintType)
struct JRPGCORE_API FJRPGHandle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint64 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OwnerTag = NAME_None;

	bool IsValid() const
	{
		return Value != 0;
	}
};

template <typename TValue>
struct TJRPGResult
{
	bool bOk = false;
	EJRPGResultCode Code = EJRPGResultCode::Rejected;
	FJRPGReason Reason = FJRPGReason::None();
	TValue Value{};

	static TJRPGResult Ok(const TValue& InValue)
	{
		TJRPGResult Result;
		Result.bOk = true;
		Result.Code = EJRPGResultCode::Success;
		Result.Value = InValue;
		return Result;
	}

	static TJRPGResult Fail(EJRPGResultCode InCode, const FJRPGReason& InReason)
	{
		TJRPGResult Result;
		Result.bOk = false;
		Result.Code = InCode;
		Result.Reason = InReason;
		return Result;
	}
};