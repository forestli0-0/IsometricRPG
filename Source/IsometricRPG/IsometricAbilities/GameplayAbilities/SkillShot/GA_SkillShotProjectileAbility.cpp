// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_SkillShotProjectileAbility.h"
#include "AbilitySystemComponent.h"
#include "Misc/DataValidation.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "NiagaraActor.h"
#include "UObject/UnrealType.h"

namespace
{
const UGA_SkillShotProjectileAbility* GetSuperProjectileDefaults(const UGA_SkillShotProjectileAbility* Ability)
{
	if (!Ability)
	{
		return nullptr;
	}

	for (UClass* SuperClass = Ability->GetClass()->GetSuperClass(); SuperClass; SuperClass = SuperClass->GetSuperClass())
	{
		if (const UGA_SkillShotProjectileAbility* SuperDefaults = Cast<UGA_SkillShotProjectileAbility>(SuperClass->GetDefaultObject()))
		{
			return SuperDefaults;
		}
	}

	return nullptr;
}
}

UGA_SkillShotProjectileAbility::UGA_SkillShotProjectileAbility()
{
	// 设置默认值
	SkillShotProjectileClass = AProjectileBase::StaticClass();
	ProjectileClass = nullptr;
	MuzzleSocketName = FName("Muzzle");
}

void UGA_SkillShotProjectileAbility::ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation)
{
	// 清理瞄准指示器
	if (ActiveAimIndicator)
	{
		ActiveAimIndicator->Destroy();
		ActiveAimIndicator = nullptr;
	}

	const TSubclassOf<AProjectileBase> ConfiguredProjectileClass = GetConfiguredProjectileClass();

	// 生成投射物
	if (ConfiguredProjectileClass)
	{
		const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
		if (!ActorInfo)
		{
			UE_LOG(LogTemp, Error, TEXT("%s: Missing ActorInfo when executing skill-shot projectile ability."), *GetName());
			return;
		}

		if (!ActorInfo->IsNetAuthority())
		{
			UE_LOG(LogTemp, Verbose, TEXT("%s: Skipping projectile spawn on non-authority instance; waiting for replicated projectile."), *GetName());
			return;
		}

		AActor* SourceActor = GetAvatarActorFromActorInfo();
		APawn* SourcePawn = Cast<APawn>(SourceActor);
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

		FVector SpawnLocation;
		FRotator SpawnRotation;
		GetLaunchTransform(Direction, SourceActor, SpawnLocation, SpawnRotation);

		AProjectileBase* SpawnedProjectile = SpawnProjectile(SpawnLocation, SpawnRotation, SourceActor, SourcePawn, SourceASC);
		
		if (SpawnedProjectile)
		{
			UE_LOG(LogTemp, Log, TEXT("SkillShot Projectile spawned: %s in direction: %s"), 
				*SpawnedProjectile->GetName(), *Direction.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s: Failed to spawn SkillShot projectile"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: No projectile class configured for skill-shot projectile ability"), *GetName());
		// 调用父类的默认实现（线性追踪）
		Super::ExecuteSkillShot(Direction, StartLocation);
	}
}

void UGA_SkillShotProjectileAbility::GetLaunchTransform(const FVector& Direction, const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation) const
{
	// 设置旋转方向
	OutRotation = Direction.Rotation();

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
		// 使用插槽位置
		OutLocation = MuzzleComponent->GetSocketLocation(MuzzleSocketName);
	}
	else if (SelfPawn)
	{
		// 使用Actor位置加向前偏移
		OutLocation = SelfPawn->GetActorLocation() + SelfPawn->GetActorForwardVector() * 50.0f;
	}
	else if (SourceActor)
	{
		// 备用方案
		OutLocation = SourceActor->GetActorLocation() + SourceActor->GetActorForwardVector() * 50.0f;
	}
	else
	{
		// 绝对备用方案
		OutLocation = FVector::ZeroVector;
		UE_LOG(LogTemp, Warning, TEXT("%s: GetLaunchTransform - SourceActor is null, defaulting OutLocation to ZeroVector."), *GetName());
	}
}

AProjectileBase* UGA_SkillShotProjectileAbility::SpawnProjectile(
	const FVector& SpawnLocation, 
	const FRotator& SpawnRotation, 
	AActor* SourceActor, 
	APawn* SourcePawn,
	UAbilitySystemComponent* SourceASC)
{
	const TSubclassOf<AProjectileBase> ConfiguredProjectileClass = GetConfiguredProjectileClass();
	if (!ConfiguredProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Skill-shot projectile class is not set in SpawnProjectile."), *GetName());
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
		ConfiguredProjectileClass, 
		SpawnLocation, 
		SpawnRotation, 
		SpawnParams);

	// 配置投射物
	if (SpawnedProjectile)
	{
		// 使用ProjectileData初始化投射物
		SpawnedProjectile->InitializeProjectile(this, ProjectileData, SourceActor, SourcePawn, SourceASC);
		
		UE_LOG(LogTemp, Log, TEXT("%s: Successfully spawned SkillShot projectile of class %s at location %s with rotation %s."), 
			*GetName(), 
			*ConfiguredProjectileClass->GetName(), 
			*SpawnLocation.ToString(), 
			*SpawnRotation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Failed to spawn SkillShot projectile of class %s at location %s with rotation %s."), 
			*GetName(), 
			*ConfiguredProjectileClass->GetName(), 
			*SpawnLocation.ToString(), 
			*SpawnRotation.ToString());
	}
	
	return SpawnedProjectile;
}

void UGA_SkillShotProjectileAbility::PostLoad()
{
	Super::PostLoad();
	SynchronizeProjectileClassFields();
}

#if WITH_EDITOR
void UGA_SkillShotProjectileAbility::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SynchronizeProjectileClassFields();
}

EDataValidationResult UGA_SkillShotProjectileAbility::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const TSubclassOf<AProjectileBase> ConfiguredProjectileClass = GetConfiguredProjectileClass();

	if (!ConfiguredProjectileClass)
	{
		Context.AddError(FText::Format(
			NSLOCTEXT("SkillShotProjectileAbility", "MissingProjectileClass", "{0}: no projectile class is configured."),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (ProjectileClass && SkillShotProjectileClass && ProjectileClass != SkillShotProjectileClass)
	{
		Context.AddError(FText::Format(
			NSLOCTEXT("SkillShotProjectileAbility", "ConflictingProjectileClass", "{0}: legacy ProjectileClass and SkillShotProjectileClass point to different projectile classes."),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

TSubclassOf<AProjectileBase> UGA_SkillShotProjectileAbility::GetConfiguredProjectileClass() const
{
	return SkillShotProjectileClass ? SkillShotProjectileClass : ProjectileClass;
}

void UGA_SkillShotProjectileAbility::SynchronizeProjectileClassFields()
{
	const UGA_SkillShotProjectileAbility* SuperDefaults = GetSuperProjectileDefaults(this);
	const TSubclassOf<AProjectileBase> SuperProjectileClass = SuperDefaults ? SuperDefaults->SkillShotProjectileClass : nullptr;
	const bool bCanonicalUsesInheritedDefault = !SkillShotProjectileClass || SkillShotProjectileClass == SuperProjectileClass;

	if (ProjectileClass)
	{
		if (bCanonicalUsesInheritedDefault)
		{
			SkillShotProjectileClass = ProjectileClass;
		}

		if (SkillShotProjectileClass == ProjectileClass)
		{
			ProjectileClass = nullptr;
		}
	}
}
