#include "GA_Ahri_E_Charm.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Character/IsometricRPGCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

UGA_Ahri_E_Charm::UGA_Ahri_E_Charm()
{
	// 设置技能基础属性
	AbilityType = EHeroAbilityType::SkillShot;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CooldownDuration = 12.0f;
	CostMagnitude = 85.0f;
	RangeToApply = 975.0f;
	
	// 设置魅惑参数
	CharmDuration = 1.75f;
	ProjectileSpeed = 1550.0f;
	ProjectileWidth = 60.0f;
	SkillShotWidth = ProjectileWidth;
	
	// 初始化伤害参数
	BaseMagicDamage = 60.0f;
	DamagePerLevel = 35.0f;
	APRatio = 0.5f;
	
	// 设置效果参数
	MovementSpeedReduction = 65.0f;
	DamageAmplification = 20.0f;

	static ConstructorHelpers::FClassFinder<AProjectileBase> ProjectileFinder(TEXT("/Game/Blueprints/Projectiles/BP_Projectile_Fireball"));
	if (ProjectileFinder.Succeeded())
	{
		SkillShotProjectileClass = ProjectileFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> CostEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_ManaCost"));
	if (CostEffectFinder.Succeeded())
	{
		CostGameplayEffectClass = CostEffectFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_AttackDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		ProjectileData.DamageEffect = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> CastMontageFinder(TEXT("/Game/Characters/Animations/AM_CastFireball"));
	if (CastMontageFinder.Succeeded())
	{
		MontageToPlay = CastMontageFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ProjectileVFXFinder(TEXT("/Game/FX/Niagara/NS_MyFireball"));
	if (ProjectileVFXFinder.Succeeded())
	{
		ProjectileVFX = ProjectileVFXFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactVFXFinder(TEXT("/Game/FX/Niagara/NS_BurstSmoke"));
	if (ImpactVFXFinder.Succeeded())
	{
		CharmHitVFX = ImpactVFXFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSoundFinder(TEXT("/Game/StarterContent/Audio/Explosion_Cue"));
	if (ImpactSoundFinder.Succeeded())
	{
		HitSound = ImpactSoundFinder.Object;
	}
	
	// 重置状态
	CurrentProjectile = nullptr;
	CharmedTarget = nullptr;
	CharmedTargetCharacter = nullptr;
}

void UGA_Ahri_E_Charm::ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation)
{
	ProjectileData.InitialSpeed = ProjectileSpeed;
	ProjectileData.MaxSpeed = ProjectileSpeed;
	ProjectileData.MaxFlyDistance = RangeToApply;
	ProjectileData.Lifespan = FMath::Max(RangeToApply / FMath::Max(ProjectileSpeed, 1.0f), 1.0f);
	ProjectileData.DamageAmount = CalculateDamage();
	ProjectileData.VisualEffect = ProjectileVFX;
	ProjectileData.ImpactEffect = CharmHitVFX;
	ProjectileData.ImpactSound = HitSound;
	ProjectileData.bEnableHoming = false;
	ProjectileData.HomingTargetActor = nullptr;

	if (CastSound)
	{
		if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo(); ActorInfo && ActorInfo->IsLocallyControlled())
		{
			if (const AActor* AvatarActor = GetAvatarActorFromActorInfo())
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), CastSound, AvatarActor->GetActorLocation());
			}
		}
	}

	Super::ExecuteSkillShot(Direction, StartLocation);
}

AProjectileBase* UGA_Ahri_E_Charm::SpawnProjectile(
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation,
	AActor* SourceActor,
	APawn* SourcePawn,
	UAbilitySystemComponent* SourceASC)
{
	AProjectileBase* SpawnedProjectile = Super::SpawnProjectile(SpawnLocation, SpawnRotation, SourceActor, SourcePawn, SourceASC);
	if (SpawnedProjectile)
	{
		SpawnedProjectile->OnProjectileHitActor.AddDynamic(this, &UGA_Ahri_E_Charm::OnCharmProjectileHit);
		CurrentProjectile = SpawnedProjectile;
	}

	return SpawnedProjectile;
}

float UGA_Ahri_E_Charm::CalculateDamage() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UIsometricRPGAttributeSetBase* LocalAttributeSet = AttributeSet;
	if (!LocalAttributeSet && ASC)
	{
		LocalAttributeSet = ASC->GetSet<UIsometricRPGAttributeSetBase>();
	}

	if (!LocalAttributeSet)
	{
		return 0.0f;
	}

	const int32 AbilityLevel = GetAbilityLevel();
	const float AbilityPower = LocalAttributeSet->GetAbilityPower();

	float TotalDamage = BaseMagicDamage + (DamagePerLevel * (AbilityLevel - 1));
	TotalDamage += AbilityPower * APRatio;

	return TotalDamage;
}

void UGA_Ahri_E_Charm::OnCharmProjectileHit(AActor* HitActor, const FHitResult& Hit)
{
	if (!HitActor)
	{
		return;
	}

	float Damage = CalculateDamage();
	const FVector ImpactLocation = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : HitActor->GetActorLocation();

	// 应用魅惑效果
	ApplyCharmEffect(HitActor);

	UE_LOG(LogTemp, Log, TEXT("阿狸魅惑命中目标: %s, 位置: %s, 伤害: %.2f"), *HitActor->GetName(), *ImpactLocation.ToString(), Damage);

	if (CurrentProjectile)
	{
		CurrentProjectile->OnProjectileHitActor.RemoveDynamic(this, &UGA_Ahri_E_Charm::OnCharmProjectileHit);
		CurrentProjectile->Destroy();
		CurrentProjectile = nullptr;
	}
}

void UGA_Ahri_E_Charm::ApplyCharmEffect(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	if (CharmedTarget && CharmedTarget != Target)
	{
		OnCharmEffectEnd(CharmedTarget);
	}

	CharmedTarget = Target;

	if (AIsometricRPGCharacter* SourceCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo()))
	{
		const float ExpireTime = GetWorld() ? (GetWorld()->GetTimeSeconds() + CharmDuration) : CharmDuration;
		SourceCharacter->RecordRecentCharmTarget(Target, ExpireTime);
	}

	CharmedTargetCharacter = Cast<AIsometricRPGCharacter>(Target);
	if (CharmedTargetCharacter && CharmedTargetCharacter->HasAuthority())
	{
		if (UCharacterMovementComponent* MovementComponent = CharmedTargetCharacter->GetCharacterMovement())
		{
			CharmedTargetOriginalMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
			const float MoveSpeedScale = FMath::Clamp(1.0f - (MovementSpeedReduction / 100.0f), 0.1f, 1.0f);
			MovementComponent->MaxWalkSpeed = FMath::Max(120.0f, CharmedTargetOriginalMaxWalkSpeed * MoveSpeedScale);
		}

		if (AActor* SourceActor = GetAvatarActorFromActorInfo())
		{
			bCharmForcedMoveActive = CharmedTargetCharacter->PathFollowingComponent
				&& CharmedTargetCharacter->PathFollowingComponent->RequestMoveToActor(SourceActor, 90.0f);
		}
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

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

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

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

	RestoreCharmedTargetMovement();

	// 清理状态
	CharmedTarget = nullptr;

	if (AIsometricRPGCharacter* SourceCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (SourceCharacter->GetRecentCharmTarget() == Target)
		{
			SourceCharacter->RecordRecentCharmTarget(nullptr, 0.0f);
		}
	}
}

void UGA_Ahri_E_Charm::RestoreCharmedTargetMovement()
{
	if (!CharmedTargetCharacter || !CharmedTargetCharacter->HasAuthority())
	{
		CharmedTargetCharacter = nullptr;
		CharmedTargetOriginalMaxWalkSpeed = 0.0f;
		bCharmForcedMoveActive = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = CharmedTargetCharacter->GetCharacterMovement();
		MovementComponent && CharmedTargetOriginalMaxWalkSpeed > 0.0f)
	{
		MovementComponent->MaxWalkSpeed = CharmedTargetOriginalMaxWalkSpeed;
	}

	if (bCharmForcedMoveActive && CharmedTargetCharacter->PathFollowingComponent)
	{
		CharmedTargetCharacter->PathFollowingComponent->StopMove(EIsometricPathFollowResult::Aborted);
	}

	CharmedTargetCharacter = nullptr;
	CharmedTargetOriginalMaxWalkSpeed = 0.0f;
	bCharmForcedMoveActive = false;
}
