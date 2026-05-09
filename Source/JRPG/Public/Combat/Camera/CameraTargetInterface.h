#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CameraTargetInterface.generated.h"

UINTERFACE()
class UCameraTargetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class JRPG_API ICameraTargetInterface
{
	GENERATED_BODY()

public:
	virtual FVector  GetCameraTargetLocation() const = 0;
	virtual FRotator GetCameraTargetRotation() const = 0;
};
