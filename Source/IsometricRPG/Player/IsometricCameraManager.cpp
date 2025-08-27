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
            // 设置摄像机的旋转角度（固定等距视角）
            OutVT.POV.Rotation = CameraAngle;

            // 以 Pawn 位置为焦点，添加微调偏移
            const FVector FocusLocation = ControlledPawn->GetActorLocation() + FocusOffset;

            // 从旋转得到方向向量，沿相反方向拉开距离
            const FVector BackVector = OutVT.POV.Rotation.Vector() * -1.f; // 朝向相机后方
            FVector DesiredCameraLocation = FocusLocation + BackVector * CameraDistance;

            // 防抖和平滑
            if (bEnablePositionSmoothing)
            {
                if (bFirstUpdate)
                {
                    PreviousCameraActualLocation = DesiredCameraLocation;
                    bFirstUpdate = false;
                }
                // 只有在位移超过阈值时才插值移动，减少细微抖动
                const float MoveDelta = FVector::Dist(PreviousCameraActualLocation, DesiredCameraLocation);
                if (MoveDelta > JitterThreshold)
                {
                    PreviousCameraActualLocation = FMath::VInterpTo(PreviousCameraActualLocation, DesiredCameraLocation, DeltaTime, CameraLagSpeed);
                }
                DesiredCameraLocation = PreviousCameraActualLocation;
            }

            OutVT.POV.Location = DesiredCameraLocation;

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
