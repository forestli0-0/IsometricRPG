// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "IsometricCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API AIsometricCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
    AIsometricCameraManager();

    // 覆盖此函数以自定义摄像机行为
    virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

    // 你可以在这里添加可配置的摄像机属性
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
    float CameraDistance = 1200.0f; // 摄像机与目标的距离

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
    FRotator CameraAngle = FRotator(-55.0f, 0.0f, 0.0f); // 摄像机的固定角度 (俯视)

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
    float CameraLagSpeed = 3.0f; // 摄像机跟随的平滑速度，降低可以减少抖动

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
    FVector FocusOffset = FVector(0.f, 0.f, 100.f); // 焦点在角色上的偏移，可以略微抬高焦点
    
    // 启用正交投影
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
    bool bUseOrthographicProjection = true;
    
    // 正交宽度（控制缩放级别）
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (EditCondition = "bUseOrthographicProjection"))
    float OrthographicWidth = 1500.0f;
    
    // 防抖动设置：启用位置平滑后处理
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|AntiJitter")
    bool bEnablePositionSmoothing = true;
    
    // 当位移小于此值时忽略，防止微小移动导致抖动
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|AntiJitter", meta = (EditCondition = "bEnablePositionSmoothing"))
    float JitterThreshold = 0.5f;

private:
    // 存储上一帧的摄像机位置，用于平滑处理
    FVector PreviousCameraActualLocation = FVector::ZeroVector;
    bool bFirstUpdate = true; // 添加首次更新标志
};
