// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_ProjectileAbility.cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_ProjectileAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Actor.h" // Added for AActor
#include "Kismet/KismetMathLibrary.h"
#include "GameplayTagContainer.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h" // 引用新的投射物基类
#include "Abilities/GameplayAbilityTypes.h" // Added for FGameplayAbilityTargetDataHandle
#include "Engine/World.h" // Added for GetWorld()

UGA_ProjectileAbility::UGA_ProjectileAbility()
{
    // 设置技能类型为投射物
    AbilityType = EHeroAbilityType::SkillShot;
    
    // 默认需要目标选择
    bRequiresTargetData = false;
    
    // 投射物类默认值
    ProjectileClass = AProjectileBase::StaticClass(); // 默认为基类，具体技能应覆盖此项
    MuzzleSocketName = FName("Muzzle");
}


void UGA_ProjectileAbility::ExecuteSkill(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
    // 获取施法者
    AActor* SelfActor = ActorInfo->AvatarActor.Get();
    if (!SelfActor || !ProjectileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot execute skill - SelfActor or ProjectileClass is invalid."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    APawn* SelfPawn = Cast<APawn>(SelfActor);
    UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
    if (!SourceASC)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot execute skill - SourceASC is invalid."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 获取发射位置和旋转
    FVector SpawnLocation;
    FRotator SpawnRotation;
    
    // 使用目标数据确定旋转
    GetLaunchTransform(TriggerEventData, SelfActor, SpawnLocation, SpawnRotation);
    
    // 生成投射物
    AProjectileBase* SpawnedProjectile = SpawnProjectile(SpawnLocation, SpawnRotation, SelfActor, SelfPawn, SourceASC);
    
    if (!SpawnedProjectile)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to spawn projectile using class %s."), *GetName(), *ProjectileClass->GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 确保在失败时结束技能
        return;
    }

    // 投射物技能通常在发射后立即结束，除非有引导时间或其他机制
    // 如果有发射动画，会由动画完成回调结束技能
    if (!MontageToPlay) 
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_ProjectileAbility::GetLaunchTransform(
    const FGameplayEventData* TriggerEventData,
    const AActor* SourceActor, 
    FVector& OutLocation, 
    FRotator& OutRotation) const
{
    // 首先尝试找到指定的插槽
    USceneComponent* MuzzleComponent = nullptr;
    const APawn* SelfPawn = Cast<APawn>(SourceActor); // 使用 Cast 而不是 const_cast
    if (SelfPawn && MuzzleSocketName != NAME_None)
    {
        // 查找具有指定插槽的组件
        TArray<USceneComponent*> Components;
        SelfPawn->GetComponents(Components);
        for (USceneComponent* Component : Components)
        {
            if (Component->DoesSocketExist(MuzzleSocketName))
            {
                MuzzleComponent = Component;
                break;
            }
        }
    }

    // 确定发射位置
    if (MuzzleComponent)
    {
        // 使用插槽位置
        OutLocation = MuzzleComponent->GetSocketLocation(MuzzleSocketName);
    }
    else if (SelfPawn) // 如果没有插槽，使用Actor位置加一个偏移
    {
        OutLocation = SelfPawn->GetActorLocation() + SelfPawn->GetActorForwardVector() * 50.0f; // 默认向前100单位
    }
    else if (SourceActor) // Fallback if SourceActor is not a Pawn but exists
    {
        OutLocation = SourceActor->GetActorLocation() + SourceActor->GetActorForwardVector() * 50.0f;
    }
    else // Absolute fallback, should ideally not happen
    {
        OutLocation = FVector::ZeroVector;
        UE_LOG(LogTemp, Warning, TEXT("%s: GetLaunchTransform - SourceActor is null, defaulting OutLocation to ZeroVector."), *GetName());
    }

    // 确定旋转方向
    FVector TargetLocation = FVector::ZeroVector;
    bool bTargetFound = false;
    if (TriggerEventData && TriggerEventData->TargetData.Data.Num() > 0) // Check IsValid(index) before Get(index)
    {
        TargetLocation = TriggerEventData->TargetData.Data[0].ToSharedRef()->GetHitResult()->Location;
        bTargetFound = true;
    }


    if (bTargetFound)
    {
        // OutLocation 是起始点 (例如，角色的当前位置)
        // TargetLocation 是目标点
        // OutRotation 应该是一个 FRotator 类型的变量

        // 1. 计算从起始点到目标点的方向向量
        FVector DirectionToTarget = TargetLocation - OutLocation;

        // 2. 通过将Z分量置零，使该方向向量变为水平方向
        DirectionToTarget.Z = 0.0f;

        // 3. (重要) 检查向量长度是否接近于零，避免方向向量为零时 Rotation() 出错
        //    如果起始点和目标点在XY平面上重合，则没有明确的水平方向。
        if (!DirectionToTarget.IsNearlyZero())
        {
            // 4. 从水平方向向量获取旋转。这个旋转的 Pitch 和 Roll 会自动为0。
            OutRotation = DirectionToTarget.Rotation();
        }
    }
    else if (SelfPawn) // 没有有效目标数据，使用Pawn的朝向
    {
        OutRotation = SelfPawn->GetControlRotation(); // 或者 GetActorRotation()
    }
    else if (SourceActor)
    {
        OutRotation = SourceActor->GetActorRotation();
    }
    else // Absolute fallback
    {
        OutRotation = FRotator::ZeroRotator;
        UE_LOG(LogTemp, Warning, TEXT("%s: GetLaunchTransform - No target data and SourceActor is null or not a Pawn, defaulting OutRotation to ZeroRotator."), *GetName());
    }
}

AProjectileBase* UGA_ProjectileAbility::SpawnProjectile(
    const FVector& SpawnLocation, 
    const FRotator& SpawnRotation, 
    AActor* SourceActor, 
    APawn* SourcePawn,
    UAbilitySystemComponent* SourceASC)
{
    if (!ProjectileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: ProjectileClass is not set in SpawnProjectile."), *GetName());
        return nullptr;
    }
    UWorld* World = GetWorld();
    if(!World)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot spawn projectile, GetWorld() returned null."), *GetName());
        return nullptr;
    }

    // 设置生成参数
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = SourceActor;       // 技能的拥有者，通常是施法者Actor
    SpawnParams.Instigator = SourcePawn;   // 技能的发起者，通常是施法者Pawn
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 生成投射物
    AProjectileBase* SpawnedProjectile = World->SpawnActor<AProjectileBase>(
        ProjectileClass, 
        SpawnLocation, 
        SpawnRotation, 
        SpawnParams);

    // 配置投射物
    if (SpawnedProjectile)
    {
        // 使用ProjectileData初始化投射物
        SpawnedProjectile->InitializeProjectile(this, ProjectileData, SourceActor, SourcePawn, SourceASC);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to spawn projectile of class %s at location %s with rotation %s."), 
            *GetName(), 
            *ProjectileClass->GetName(), 
            *SpawnLocation.ToString(), 
            *SpawnRotation.ToString());
    }
    
    return SpawnedProjectile;
}
