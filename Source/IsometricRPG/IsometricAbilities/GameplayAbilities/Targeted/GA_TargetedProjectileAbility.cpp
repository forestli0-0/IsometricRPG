// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_TargetedProjectileAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"

UGA_TargetedProjectileAbility::UGA_TargetedProjectileAbility()
{
	// 设置默认值
	ProjectileClass = AProjectileBase::StaticClass();
	MuzzleSocketName = FName("Muzzle");
}

void UGA_TargetedProjectileAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 获取施法者
	AActor* SelfActor = GetAvatarActorFromActorInfo();
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
	GetLaunchTransform(TriggerEventData, SelfActor, SpawnLocation, SpawnRotation);
	
	// 生成投射物
	AProjectileBase* SpawnedProjectile = SpawnProjectile(SpawnLocation, SpawnRotation, SelfActor, SelfPawn, SourceASC, TriggerEventData);
	
	if (!SpawnedProjectile)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Failed to spawn targeted projectile using class %s."), *GetName(), *ProjectileClass->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%s: Successfully executed targeted projectile ability."), *GetName());

	// 投射物技能通常在发射后立即结束，除非有引导时间或其他机制
	if (!MontageToPlay) 
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_TargetedProjectileAbility::GetLaunchTransform(const FGameplayEventData* TriggerEventData, const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation) const
{
	// 首先尝试找到指定的插槽
	USceneComponent* MuzzleComponent = nullptr;
	const APawn* SelfPawn = Cast<APawn>(SourceActor);
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
		OutLocation = MuzzleComponent->GetSocketLocation(MuzzleSocketName);
	}
	else if (SelfPawn)
	{
		OutLocation = SelfPawn->GetActorLocation() + SelfPawn->GetActorForwardVector() * 50.0f;
	}
	else if (SourceActor)
	{
		OutLocation = SourceActor->GetActorLocation() + SourceActor->GetActorForwardVector() * 50.0f;
	}
	else
	{
		OutLocation = FVector::ZeroVector;
		UE_LOG(LogTemp, Warning, TEXT("%s: GetLaunchTransform - SourceActor is null, defaulting OutLocation to ZeroVector."), *GetName());
	}

	// 确定旋转方向（朝向目标）
	FVector TargetLocation = FVector::ZeroVector;
	bool bTargetFound = false;
	
	if (TriggerEventData && TriggerEventData->TargetData.Data.Num() > 0)
	{
		const FHitResult* HitResult = TriggerEventData->TargetData.Data[0]->GetHitResult();
		if (HitResult && HitResult->GetActor())
		{
			TargetLocation = HitResult->GetActor()->GetActorLocation();
			//DrawDebugSphere(GetWorld(), TargetLocation, 20.0f, 12, FColor::Green, false, 5.0f);
			bTargetFound = true;
		}
	}
	else if (TriggerEventData && TriggerEventData->Target)
	{
		auto TargetActor = TriggerEventData->Target;
		if (TargetActor)
		{
			TargetLocation = TargetActor->GetActorLocation();
			//DrawDebugSphere(GetWorld(), TargetLocation, 20.0f, 12, FColor::Green, false, 5.0f);
			bTargetFound = true;
		}
	}
	if (bTargetFound)
	{
		// 计算从起始点到目标的方向
		FVector DirectionToTarget = TargetLocation - OutLocation;
		//DrawDebugDirectionalArrow(GetWorld(), OutLocation, TargetLocation, 50.0f, FColor::Red, false, 5.0f);
		DirectionToTarget.Z = 0.0f; // 保持水平方向

		if (!DirectionToTarget.IsNearlyZero())
		{
			OutRotation = DirectionToTarget.Rotation();
		}
		else
		{
			OutRotation = SourceActor ? SourceActor->GetActorRotation() : FRotator::ZeroRotator;
		}
	}
	else if (SelfPawn)
	{
		OutRotation = SelfPawn->GetControlRotation();
	}
	else if (SourceActor)
	{
		OutRotation = SourceActor->GetActorRotation();
	}
	else
	{
		OutRotation = FRotator::ZeroRotator;
		UE_LOG(LogTemp, Warning, TEXT("%s: GetLaunchTransform - No target data and SourceActor is null, defaulting OutRotation to ZeroRotator."), *GetName());
	}
}

AProjectileBase* UGA_TargetedProjectileAbility::SpawnProjectile(
	const FVector& SpawnLocation, 
	const FRotator& SpawnRotation, 
	AActor* SourceActor, 
	APawn* SourcePawn,
	UAbilitySystemComponent* SourceASC,
	const FGameplayEventData* TriggerEventData)
{
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: ProjectileClass is not set in SpawnProjectile."), *GetName());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Cannot spawn projectile, GetWorld() returned null."), *GetName());
		return nullptr;
	}

	// 设置生成参数
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor;
	SpawnParams.Instigator = SourcePawn;
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
		
		UE_LOG(LogTemp, Log, TEXT("%s: Successfully spawned targeted projectile of class %s at location %s with rotation %s."), 
			*GetName(), 
			*ProjectileClass->GetName(), 
			*SpawnLocation.ToString(), 
			*SpawnRotation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Failed to spawn targeted projectile of class %s at location %s with rotation %s."), 
			*GetName(), 
			*ProjectileClass->GetName(), 
			*SpawnLocation.ToString(), 
			*SpawnRotation.ToString());
	}
	
	return SpawnedProjectile;
}
