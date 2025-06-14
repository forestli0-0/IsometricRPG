#include "GA_Ahri_R_SpiritRush.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"

UGA_Ahri_R_SpiritRush::UGA_Ahri_R_SpiritRush()
{
	// 设置技能基础属性
	AbilityType = EHeroAbilityType::SelfCast;
	CooldownDuration = 100.0f;
	CostMagnitude = 100.0f;
	
	// 初始化大招参数
	MaxCharges = 3;
	DashDistance = 450.0f;
	DashSpeed = 2200.0f;
	SpiritOrbCount = 3;
	SpiritOrbRange = 550.0f;
	SpiritOrbSpeed = 1600.0f;
	
	// 初始化伤害参数
	BaseMagicDamage = 60.0f;
	DamagePerLevel = 45.0f;
	APRatio = 0.35f;
	
	// 设置充能窗口和伤害递减
	ChargeWindowDuration = 10.0f;
	DamageReductionPerHit = 10.0f;
	
	// 重置状态
	CurrentCharges = 0;
	bInChargeWindow = false;
	DashTask = nullptr;
}

void UGA_Ahri_R_SpiritRush::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 检查是否可以再次使用
	if (bInChargeWindow && CurrentCharges >= MaxCharges)
	{
		// 已经用完所有次数
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 调用父类的激活逻辑
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Ahri_R_SpiritRush::ExecuteSelfCast()
{
	// 播放施法音效
	if (CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CastSound, GetAvatarActorFromActorInfo()->GetActorLocation());
	}

	// 如果不在充能窗口内，开始新的充能周期
	if (!bInChargeWindow)
	{
		bInChargeWindow = true;
		CurrentCharges = 0;
		EnemyHitCounts.Empty();
		
		// 设置充能窗口定时器
		GetWorld()->GetTimerManager().SetTimer(
			ChargeWindowTimerHandle,
			this,
			&UGA_Ahri_R_SpiritRush::OnChargeWindowExpired,
			ChargeWindowDuration,
			false
		);
	}

	// 增加使用次数
	CurrentCharges++;

	// 执行瞬移
	ExecuteDash();

	UE_LOG(LogTemp, Log, TEXT("阿狸大招使用第 %d 次"), CurrentCharges);
}

float UGA_Ahri_R_SpiritRush::CalculateDamage(int32 HitCount) const
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

	// 应用伤害递减（如果同一个敌人被多次命中）
	if (HitCount > 0)
	{
		float DamageReduction = FMath::Min(HitCount * DamageReductionPerHit, 80.0f); // 最多减少80%
		TotalDamage *= (1.0f - DamageReduction / 100.0f);
	}

	return TotalDamage;
}

void UGA_Ahri_R_SpiritRush::ExecuteDash()
{
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	if (!AhriActor)
	{
		return;
	}

	// 获取瞬移目标位置
	FVector TargetLocation = GetDashTargetLocation();
	FVector CurrentLocation = AhriActor->GetActorLocation();

	// 播放瞬移开始特效
	if (DashStartVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DashStartVFX,
			CurrentLocation,
			FRotator::ZeroRotator
		);
	}

	// 播放瞬移音效
	if (DashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashSound, CurrentLocation);
	}

	// 执行瞬移（这里简化为直接设置位置，实际应该使用平滑移动）
	AhriActor->SetActorLocation(TargetLocation);

	// 播放瞬移结束特效
	if (DashEndVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DashEndVFX,
			TargetLocation,
			FRotator::ZeroRotator
		);
	}

	// 瞬移完成后发射狐灵火球
	FireSpiritOrbs();

	UE_LOG(LogTemp, Log, TEXT("阿狸瞬移完成，从 %s 到 %s"), 
		*CurrentLocation.ToString(), *TargetLocation.ToString());
}

FVector UGA_Ahri_R_SpiritRush::GetDashTargetLocation() const
{
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	if (!AhriActor)
	{
		return FVector::ZeroVector;
	}

	FVector CurrentLocation = AhriActor->GetActorLocation();
	
	// TODO: 获取鼠标位置或输入方向
	// 这里简化为向前方瞬移
	FVector ForwardDirection = AhriActor->GetActorForwardVector();
	FVector TargetLocation = CurrentLocation + (ForwardDirection * DashDistance);

	// 检查目标位置是否有效（没有碰撞）
	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CurrentLocation,
		TargetLocation,
		ECC_WorldStatic
	);

	if (bHit)
	{
		// 如果有碰撞，调整到碰撞点前的安全位置
		TargetLocation = HitResult.Location - (ForwardDirection * 50.0f);
	}

	return TargetLocation;
}

void UGA_Ahri_R_SpiritRush::FireSpiritOrbs()
{
	// 寻找狐灵火球的目标
	TArray<AActor*> Targets = FindSpiritOrbTargets();
	
	if (Targets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("阿狸大招：未找到狐灵火球目标"));
		
		// 如果已经用完所有次数，结束技能
		if (CurrentCharges >= MaxCharges)
		{
			OnChargeWindowExpired();
		}
		return;
	}

	// 播放狐灵火球发射音效
	if (SpiritOrbFireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpiritOrbFireSound, GetAvatarActorFromActorInfo()->GetActorLocation());
	}

	// 发射狐灵火球
	FVector AhriLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	
	for (int32 i = 0; i < FMath::Min(SpiritOrbCount, Targets.Num()); i++)
	{
		AActor* Target = Targets[i];
		if (!Target)
		{
			continue;
		}

		// 创建狐灵火球投射物
		if (SkillShotProjectileClass)  // 使用基类的投射物类
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetAvatarActorFromActorInfo();
			SpawnParams.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			FVector Direction = (Target->GetActorLocation() - AhriLocation).GetSafeNormal();
			FVector StartLocation = AhriLocation + FVector(0, 0, 50); // 稍微抬高

			AProjectileBase* SpiritOrb = GetWorld()->SpawnActor<AProjectileBase>(
				SkillShotProjectileClass,
				StartLocation,
				Direction.Rotation(),
				SpawnParams
			);

			if (SpiritOrb)
			{
				ActiveSpiritOrbs.Add(SpiritOrb);

				// 设置狐灵火球的属性
				// SpiritOrb->SetProjectileSpeed(SpiritOrbSpeed);
				// SpiritOrb->SetHomingTarget(Target);

				// 绑定狐灵火球的事件回调
				// SpiritOrb->OnProjectileHit.AddDynamic(this, &UGA_Ahri_R_SpiritRush::OnSpiritOrbHitTarget);

				// 播放狐灵火球特效
				if (SpiritOrbVFX)
				{
					UNiagaraFunctionLibrary::SpawnSystemAttached(
						SpiritOrbVFX,
						SpiritOrb->GetRootComponent(),
						NAME_None,
						FVector::ZeroVector,
						FRotator::ZeroRotator,
						EAttachLocation::KeepRelativeOffset,
						true
					);
				}

				UE_LOG(LogTemp, Log, TEXT("阿狸大招发射狐灵火球 %d，目标: %s"), i, *Target->GetName());
			}
		}
	}

	// 如果已经用完所有次数，准备结束技能
	if (CurrentCharges >= MaxCharges)
	{
		// 延迟一点时间等待所有狐灵火球完成
		FTimerHandle DelayEndTimer;
		GetWorld()->GetTimerManager().SetTimer(
			DelayEndTimer,
			this,
			&UGA_Ahri_R_SpiritRush::OnChargeWindowExpired,
			2.0f,  // 等待2秒
			false
		);
	}
}

TArray<AActor*> UGA_Ahri_R_SpiritRush::FindSpiritOrbTargets() const
{
	TArray<AActor*> ValidTargets;
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	
	if (!AhriActor)
	{
		return ValidTargets;
	}

	// TODO: 优先选择阿狸当前攻击的目标
	// AActor* CurrentTarget = AhriActor->GetCurrentTarget();

	// 寻找范围内的所有敌人
	FVector AhriLocation = AhriActor->GetActorLocation();
	TArray<AActor*> FoundActors;
	
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AhriLocation,
		SpiritOrbRange,
		TArray<TEnumAsByte<EObjectTypeQuery>>(),  // TODO: 设置正确的对象类型
		nullptr,  // 暂不过滤类
		TArray<AActor*>{AhriActor},  // 排除阿狸自己
		FoundActors
	);

	// 按距离排序并选择最近的目标
	FoundActors.Sort([AhriLocation](const AActor& A, const AActor& B) {
		float DistA = FVector::Dist(AhriLocation, A.GetActorLocation());
		float DistB = FVector::Dist(AhriLocation, B.GetActorLocation());
		return DistA < DistB;
	});

	// 选择前几个目标
	for (int32 i = 0; i < FMath::Min(SpiritOrbCount, FoundActors.Num()); i++)
	{
		// TODO: 检查是否为敌人
		// if (IsEnemy(FoundActors[i]))
		// {
			ValidTargets.Add(FoundActors[i]);
		// }
	}

	return ValidTargets;
}

void UGA_Ahri_R_SpiritRush::OnSpiritOrbHitTarget(AActor* HitActor, int32 OrbIndex)
{
	if (!HitActor)
	{
		return;
	}

	// 获取该敌人之前被命中的次数
	int32 HitCount = EnemyHitCounts.FindRef(HitActor);
	
	// 计算伤害（考虑伤害递减）
	float Damage = CalculateDamage(HitCount);

	// 更新命中次数
	EnemyHitCounts.Add(HitActor, HitCount + 1);

	// 应用伤害（这里需要使用GAS的伤害系统）
	// TODO: 创建并应用伤害的GameplayEffect

	// 播放命中特效
	if (SpiritOrbHitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SpiritOrbHitVFX,
			HitActor->GetActorLocation(),
			FRotator::ZeroRotator
		);
	}

	// 播放命中音效
	if (SpiritOrbHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpiritOrbHitSound, HitActor->GetActorLocation());
	}

	UE_LOG(LogTemp, Log, TEXT("阿狸大招狐灵火球 %d 命中目标: %s, 伤害: %.2f (第%d次命中)"), 
		OrbIndex, *HitActor->GetName(), Damage, HitCount + 1);
}

bool UGA_Ahri_R_SpiritRush::CanUseAgain() const
{
	return bInChargeWindow && CurrentCharges < MaxCharges;
}

void UGA_Ahri_R_SpiritRush::ResetAbilityState()
{
	bInChargeWindow = false;
	CurrentCharges = 0;
	EnemyHitCounts.Empty();
	ActiveSpiritOrbs.Empty();
	
	// 清理定时器
	GetWorld()->GetTimerManager().ClearTimer(ChargeWindowTimerHandle);
	
	// 清理瞬移任务
	if (DashTask)
	{
		DashTask = nullptr;
	}
}

void UGA_Ahri_R_SpiritRush::OnChargeWindowExpired()
{
	UE_LOG(LogTemp, Log, TEXT("阿狸大招充能窗口结束，总共使用了 %d 次"), CurrentCharges);

	// 重置状态
	ResetAbilityState();

	// 结束技能
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
} 