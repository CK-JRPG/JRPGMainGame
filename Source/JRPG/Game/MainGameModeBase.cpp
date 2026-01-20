// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameModeBase.h"

#include "JRPG/Player/CombatPlayerController.h"

AMainGameModeBase::AMainGameModeBase()
{
	PlayerControllerClass = ACombatPlayerController::StaticClass();
}
