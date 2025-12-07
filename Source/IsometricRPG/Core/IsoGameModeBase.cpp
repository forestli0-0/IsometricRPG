// Fill out your copyright notice in the Description page of Project Settings.


#include "IsoGameModeBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Character/IsometricRPGCharacter.h"
#include "Player/IsometricPlayerController.h"
#include "Character/IsoPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AIsoGameModeBase::AIsoGameModeBase()
{
	// 绑定默认类，避免需要在关卡中放置 PlayerController
	DefaultPawnClass = AIsometricRPGCharacter::StaticClass();
	PlayerControllerClass = AIsometricPlayerController::StaticClass();
	PlayerStateClass = AIsoPlayerState::StaticClass();
}

void AIsoGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 调试日志，确认多人PIE下是否正确登录并获得 Pawn
	if (NewPlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("[GameMode] 玩家登录: 控制器=%s, 当前Pawn=%s, 是否本地=%s"),
			*NewPlayer->GetName(),
			NewPlayer->GetPawn() ? *NewPlayer->GetPawn()->GetName() : TEXT("无"),
			NewPlayer->IsLocalController() ? TEXT("是") : TEXT("否"));
	}
}

