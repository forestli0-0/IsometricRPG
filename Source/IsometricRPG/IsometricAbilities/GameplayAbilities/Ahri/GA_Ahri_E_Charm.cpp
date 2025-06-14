#include "GA_Ahri_E_Charm.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UGA_Ahri_E_Charm::UGA_Ahri_E_Charm()
{
	// 设置技能基础属性
	AbilityType = EHeroAbilityType::Targeted;
	CooldownDuration = 12.0f;
	CostMagnitude = 85.0f;
	RangeToApply = 975.0f;
	
	// 设置魅惑参数
	CharmDuration = 1.75f;
	ProjectileSpeed = 1550.0f;
	ProjectileWidth = 60.0f;
	
	// 初始化伤害参数
	BaseMagicDamage = 60.0f;
	DamagePerLevel = 35.0f;
	APRatio = 0.5f;
	
	// 设置效果参数
	MovementSpeedReduction = 65.0f;
	DamageAmplification = 20.0f;
	
	// 重置状态
	CurrentProjectile = nullptr;
	CharmedTarget = nullptr;
}

void UGA_Ahri_E_Charm::ExecuteTargetedProjectile(AActor* Target, const FVector& TargetLocation)
{
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	if (!AhriActor)
	{
		return;
	}

	// 播放施法音效
	if (CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CastSound, AhriActor->GetActorLocation());
	}

	// 计算发射位置和方向
	FVector StartLocation = AhriActor->GetActorLocation() + FVector(0, 0, 50); // 稍微抬高一点
	FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();

	// 创建魅惑投射物
	if (ProjectileClass)  // 使用基类的投射物类
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = AhriActor;
		SpawnParams.Instigator = Cast<APawn>(AhriActor);
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentProjectile = GetWorld()->SpawnActor<AProjectileBase>(
			ProjectileClass,
			StartLocation,
			Direction.Rotation(),
			SpawnParams
		);

		if (CurrentProjectile)
		{
			// 设置投射物的属性
			// CurrentProjectile->SetProjectileSpeed(ProjectileSpeed);
			// CurrentProjectile->SetProjectileWidth(ProjectileWidth);
			// CurrentProjectile->SetMaxRange(RangeToApply);

			// 绑定投射物的事件回调
			// CurrentProjectile->OnProjectileHit.AddDynamic(this, &UGA_Ahri_E_Charm::OnCharmProjectileHit);

			// 播放投射物特效
			if (ProjectileVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(
					ProjectileVFX,
					CurrentProjectile->GetRootComponent(),
					NAME_None,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset,
					true
				);
			}

			UE_LOG(LogTemp, Log, TEXT("阿狸魅惑技能发射成功，目标位置: %s"), *TargetLocation.ToString());
		}
	}
}

float UGA_Ahri_E_Charm::CalculateDamage() const
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

void UGA_Ahri_E_Charm::OnCharmProjectileHit(AActor* HitActor)
{
	if (!HitActor)
	{
		return;
	}

	// 播放命中音效
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitActor->GetActorLocation());
	}

	// 播放命中特效
	if (CharmHitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			CharmHitVFX,
			HitActor->GetActorLocation(),
			FRotator::ZeroRotator
		);
	}

	// 计算伤害
	float Damage = CalculateDamage();

	// 应用伤害（这里需要使用GAS的伤害系统）
	// TODO: 创建并应用伤害的GameplayEffect

	// 应用魅惑效果
	ApplyCharmEffect(HitActor);

	UE_LOG(LogTemp, Log, TEXT("阿狸魅惑命中目标: %s, 伤害: %.2f"), *HitActor->GetName(), Damage);

	// 清理投射物
	CurrentProjectile = nullptr;

	// 结束技能（魅惑效果会在指定时间后自动结束）
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Ahri_E_Charm::ApplyCharmEffect(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	CharmedTarget = Target;

	// 获取目标的ASC
	UAbilitySystemComponent* TargetASC = nullptr;
	// TargetASC = Target->GetAbilitySystemComponent();  // TODO: 获取目标的ASC

	if (TargetASC)
	{
		// 应用魅惑效果
		if (CharmEffectClass)
		{
			FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
			EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

			FGameplayEffectSpecHandle CharmSpecHandle = TargetASC->MakeOutgoingSpec(
				CharmEffectClass,
				1.0f,  // 技能等级
				EffectContext
			);

			if (CharmSpecHandle.IsValid())
			{
				// 设置魅惑持续时间
				CharmSpecHandle.Data->SetDuration(CharmDuration, false);
				
				CharmEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*CharmSpecHandle.Data);
			}
		}

		// 应用伤害增幅效果
		if (DamageAmplificationEffectClass)
		{
			FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
			EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

			FGameplayEffectSpecHandle AmplificationSpecHandle = TargetASC->MakeOutgoingSpec(
				DamageAmplificationEffectClass,
				1.0f,  // 技能等级
				EffectContext
			);

			if (AmplificationSpecHandle.IsValid())
			{
				// 设置伤害增幅持续时间
				AmplificationSpecHandle.Data->SetDuration(CharmDuration, false);
				
				DamageAmplificationHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*AmplificationSpecHandle.Data);
			}
		}
	}

	// 播放魅惑状态特效
	if (CharmStatusVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			CharmStatusVFX,
			Target->GetRootComponent(),
			NAME_None,
			FVector(0, 0, 100),  // 在目标头顶显示
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// 设置定时器来处理魅惑效果结束
	FTimerHandle CharmEndTimer;
	FTimerDelegate CharmEndDelegate;
	CharmEndDelegate.BindUFunction(this, FName("OnCharmEffectEnd"), Target);
	
	GetWorld()->GetTimerManager().SetTimer(
		CharmEndTimer,
		CharmEndDelegate,
		CharmDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("阿狸对 %s 应用魅惑效果，持续时间: %.2f秒"), *Target->GetName(), CharmDuration);
}

void UGA_Ahri_E_Charm::OnCharmEffectEnd(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("阿狸对 %s 的魅惑效果结束"), *Target->GetName());

	// 获取目标的ASC
	UAbilitySystemComponent* TargetASC = nullptr;
	// TargetASC = Target->GetAbilitySystemComponent();  // TODO: 获取目标的ASC

	if (TargetASC)
	{
		// 移除魅惑效果
		if (CharmEffectHandle.IsValid())
		{
			TargetASC->RemoveActiveGameplayEffect(CharmEffectHandle);
			CharmEffectHandle = FActiveGameplayEffectHandle();
		}

		// 移除伤害增幅效果
		if (DamageAmplificationHandle.IsValid())
		{
			TargetASC->RemoveActiveGameplayEffect(DamageAmplificationHandle);
			DamageAmplificationHandle = FActiveGameplayEffectHandle();
		}
	}

	// 清理状态
	CharmedTarget = nullptr;
} 