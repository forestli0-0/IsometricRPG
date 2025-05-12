// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/IsometricCameraManager.h"
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
            FVector PawnFocusLocation = ControlledPawn->GetActorLocation() + FocusOffset;

            // 计算摄像机的目标位置
            // 从焦点位置沿着摄像机角度的反方向拉远CameraDistance的距离
            FVector TargetCameraLocation = PawnFocusLocation - CameraAngle.Vector() * CameraDistance;

            // 防抖动处理：如果启用了位置平滑
            if (bEnablePositionSmoothing)
            {
                // 首帧初始化：直接设置摄像机位置，避免插值跳跃
                if (bFirstUpdate)
                {
                    OutVT.POV.Location = TargetCameraLocation;
                    PreviousCameraActualLocation = TargetCameraLocation;
                    bFirstUpdate = false;
                }
                else
                {
                    // 计算当前目标与上一帧实际位置的差异
                    FVector DeltaLocation = TargetCameraLocation - PreviousCameraActualLocation;

                    if (DeltaLocation.SizeSquared() < JitterThreshold * JitterThreshold)
                    {
                        TargetCameraLocation = PreviousCameraActualLocation;
                    }

                    // 应用插值
                    OutVT.POV.Location = FMath::VInterpTo(
                        OutVT.POV.Location,
                        TargetCameraLocation,
                        DeltaTime,
                        CameraLagSpeed
                    );

                    // 更新记录为插值后的实际位置
                    PreviousCameraActualLocation = OutVT.POV.Location;
                }
            }
            else
            {
                // 未启用平滑时直接更新
                OutVT.POV.Location = TargetCameraLocation;
                PreviousCameraActualLocation = TargetCameraLocation;
            }

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
