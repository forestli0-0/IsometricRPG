#include "GA_Ahri_W_Foxfire.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

UGA_Ahri_W_Foxfire::UGA_Ahri_W_Foxfire()
{
	// 设置技能基础属性
	AbilityType = EHeroAbilityType::SelfCast;
	CooldownDuration = 9.0f;
	CostMagnitude = 55.0f;
	
	// 初始化狐火球参数
	FoxfireCount = 3;
	SearchRange = 550.0f;
	FoxfireSpeed = 1500.0f;
	OrbitRadius = 150.0f;
	OrbitSpeed = 3.0f;
	
	// 初始化伤害参数
	BaseMagicDamage = 40.0f;
	DamagePerLevel = 25.0f;
	APRatio = 0.3f;
	
	// 设置持续时间和发射间隔
	FoxfireDuration = 5.0f;
	FireInterval = 0.25f;
	
	// 重置状态
	FiredFoxfireCount = 0;
	OrbitEffectComponent = nullptr;
}

void UGA_Ahri_W_Foxfire::ExecuteSelfCast()
{
	// 播放施法音效
	if (CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CastSound, GetAvatarActorFromActorInfo()->GetActorLocation());
	}

	// 清空之前的状态
	ActiveFoxfires.Empty();
	HitActors.Empty();
	FiredFoxfireCount = 0;

	// 创建环绕特效
	if (OrbitVFX)
	{
		OrbitEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			OrbitVFX,
			GetAvatarActorFromActorInfo()->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// 开始创建狐火球
	CreateFoxfires();
}

float UGA_Ahri_W_Foxfire::CalculateDamage() const
{
	if (!AttributeSet)
	{
		return 0.0f;
	}

	// 获取技能等级（假设从技能等级系统获取，这里暂时使用固定值）
	int32 AbilityLevel = 1; // TODO: 从技能系统获取实际等级
	
	// 获取法术强度
	float AbilityPower = 0.0f; // TODO: 从属性集获取法术强度
	// float AbilityPower = AttributeSet->GetAbilityPower();

	// 计算基础伤害
	float TotalDamage = BaseMagicDamage + (DamagePerLevel * (AbilityLevel - 1));
	
	// 添加法术强度加成
	TotalDamage += AbilityPower * APRatio;

	return TotalDamage;
}

void UGA_Ahri_W_Foxfire::CreateFoxfires()
{
	// 为每个狐火球预创建位置（环绕在阿狸周围）
	FVector AhriLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	
	for (int32 i = 0; i < FoxfireCount; i++)
	{
		// 计算狐火球的初始位置（等距离环绕）
		float Angle = (2.0f * PI * i) / FoxfireCount;
		FVector Offset = FVector(
			FMath::Cos(Angle) * OrbitRadius,
			FMath::Sin(Angle) * OrbitRadius,
			0.0f
		);
		FVector FoxfireLocation = AhriLocation + Offset;

		// 延迟发射狐火球
		FTimerDelegate FireDelegate;
		FireDelegate.BindUFunction(this, FName("FireFoxfire"), i);
		
		GetWorld()->GetTimerManager().SetTimer(
			FireTimerHandle,
			FireDelegate,
			FireInterval * i,
			false
		);
	}

	UE_LOG(LogTemp, Log, TEXT("阿狸狐火技能：准备发射 %d 个狐火球"), FoxfireCount);
}

void UGA_Ahri_W_Foxfire::FireFoxfire(int32 FoxfireIndex)
{
	// 寻找目标
	AActor* Target = FindFoxfireTarget();
	
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("阿狸狐火球 %d：未找到有效目标"), FoxfireIndex);
		FiredFoxfireCount++;
		
		// 如果所有狐火球都已处理完成
		if (FiredFoxfireCount >= FoxfireCount)
		{
			OnAllFoxfiresComplete();
		}
		return;
	}

	// 播放发射音效
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, GetAvatarActorFromActorInfo()->GetActorLocation());
	}

	// 计算狐火球的发射位置
	FVector AhriLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	float Angle = (2.0f * PI * FoxfireIndex) / FoxfireCount;
	FVector Offset = FVector(
		FMath::Cos(Angle) * OrbitRadius,
		FMath::Sin(Angle) * OrbitRadius,
		50.0f  // 稍微抬高一点
	);
	FVector StartLocation = AhriLocation + Offset;

	// 创建狐火球投射物
	if (SkillShotProjectileClass)  // 使用基类的投射物类
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetAvatarActorFromActorInfo();
		SpawnParams.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 计算向目标的方向
		FVector Direction = (Target->GetActorLocation() - StartLocation).GetSafeNormal();

		AProjectileBase* Foxfire = GetWorld()->SpawnActor<AProjectileBase>(
			SkillShotProjectileClass,
			StartLocation,
			Direction.Rotation(),
			SpawnParams
		);

		if (Foxfire)
		{
			ActiveFoxfires.Add(Foxfire);

			// 设置狐火球的属性
			// Foxfire->SetProjectileSpeed(FoxfireSpeed);
			// Foxfire->SetHomingTarget(Target);  // 如果支持追踪目标
			// Foxfire->SetProjectileRadius(50.0f);

			// 绑定狐火球的事件回调
			// Foxfire->OnProjectileHit.AddDynamic(this, &UGA_Ahri_W_Foxfire::OnFoxfireHitTarget);

			// 播放狐火球特效
			if (FoxfireVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(
					FoxfireVFX,
					Foxfire->GetRootComponent(),
					NAME_None,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset,
					true
				);
			}

			UE_LOG(LogTemp, Log, TEXT("阿狸狐火球 %d 发射成功，目标: %s"), FoxfireIndex, *Target->GetName());
		}
	}

	FiredFoxfireCount++;
}

AActor* UGA_Ahri_W_Foxfire::FindFoxfireTarget() const
{
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	if (!AhriActor)
	{
		return nullptr;
	}

	// TODO: 优先选择阿狸当前攻击的目标
	// AActor* CurrentTarget = AhriActor->GetCurrentTarget();
	// if (CurrentTarget && !HitActors.Contains(CurrentTarget))
	// {
	//     float Distance = FVector::Dist(AhriActor->GetActorLocation(), CurrentTarget->GetActorLocation());
	//     if (Distance <= SearchRange)
	//     {
	//         return CurrentTarget;
	//     }
	// }

	// 寻找范围内最近的敌人
	FVector AhriLocation = AhriActor->GetActorLocation();
	TArray<AActor*> FoundActors;
	
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AhriLocation,
		SearchRange,
		TArray<TEnumAsByte<EObjectTypeQuery>>(),  // TODO: 设置正确的对象类型
		nullptr,  // 暂不过滤类
		TArray<AActor*>{AhriActor},  // 排除阿狸自己
		FoundActors
	);

	AActor* NearestEnemy = nullptr;
	float NearestDistance = FLT_MAX;

	for (AActor* Actor : FoundActors)
	{
		// 检查是否已经被命中过
		if (HitActors.Contains(Actor))
		{
			continue;
		}

		// TODO: 检查是否为敌人
		// if (!IsEnemy(Actor))
		// {
		//     continue;
		// }

		float Distance = FVector::Dist(AhriLocation, Actor->GetActorLocation());
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			NearestEnemy = Actor;
		}
	}

	return NearestEnemy;
}

void UGA_Ahri_W_Foxfire::OnFoxfireHitTarget(AActor* HitActor, int32 FoxfireIndex)
{
	if (!HitActor || HitActors.Contains(HitActor))
	{
		return;
	}

	// 添加到已命中列表
	HitActors.Add(HitActor);

	// 计算伤害
	float Damage = CalculateDamage();

	// 应用伤害（这里需要使用GAS的伤害系统）
	// TODO: 创建并应用伤害的GameplayEffect

	// 播放命中特效
	if (HitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			HitVFX,
			HitActor->GetActorLocation(),
			FRotator::ZeroRotator
		);
	}

	// 播放命中音效
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitActor->GetActorLocation());
	}

	UE_LOG(LogTemp, Log, TEXT("阿狸狐火球 %d 命中目标: %s, 伤害: %.2f"), 
		FoxfireIndex, *HitActor->GetName(), Damage);

	// 检查是否所有狐火球都已完成
	if (FiredFoxfireCount >= FoxfireCount)
	{
		OnAllFoxfiresComplete();
	}
}

void UGA_Ahri_W_Foxfire::OnAllFoxfiresComplete()
{
	UE_LOG(LogTemp, Log, TEXT("阿狸狐火技能完成"));

	// 清理环绕特效
	if (OrbitEffectComponent)
	{
		OrbitEffectComponent->DestroyComponent();
		OrbitEffectComponent = nullptr;
	}

	// 清理状态
	ActiveFoxfires.Empty();
	HitActors.Empty();
	FiredFoxfireCount = 0;

	// 清理定时器
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);

	// 结束技能
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
} 