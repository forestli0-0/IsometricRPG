// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IsoGameModeBase.generated.h"

// 正确的前向声明应放在类外
class APlayerController;

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API AIsoGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	AIsoGameModeBase();

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
