// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricCameraManager.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"

AIsometricCameraManager::AIsometricCameraManager()
{
}

void AIsometricCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
    Super::UpdateViewTarget(OutVT, DeltaTime); // 调用父类实现非常重要

    APlayerController* OwningPC = GetOwningPlayerController();
    if (OwningPC)
    {
        APawn* ControlledPawn = OwningPC->GetPawn();
        if (ControlledPawn)
        {
            // 设置摄像机的旋转角度
            OutVT.POV.Rotation = CameraAngle;

            // 设置正交投影
            if (bUseOrthographicProjection)
            {
                // 启用正交模式
                OutVT.POV.ProjectionMode = ECameraProjectionMode::Orthographic;
                // 设置正交宽度
                OutVT.POV.OrthoWidth = OrthographicWidth;
            }
            else
            {
                // 使用透视模式
                OutVT.POV.ProjectionMode = ECameraProjectionMode::Perspective;
                // 可以设置FOV等其他参数
                // OutVT.POV.FOV = DesiredFOV;
            }
        }
    }
}
