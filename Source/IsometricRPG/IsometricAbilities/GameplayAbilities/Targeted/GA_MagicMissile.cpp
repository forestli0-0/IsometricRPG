// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_MagicMissile.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UGA_MagicMissile::UGA_MagicMissile()
{
	// 设置技能类型为Targeted
	AbilityType = EHeroAbilityType::Targeted;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	TargetActorClass = AGATA_CursorTrace::StaticClass();

	static ConstructorHelpers::FClassFinder<AProjectileBase> ProjectileFinder(TEXT("/Game/Blueprints/Projectiles/BP_Projectile_Fireball"));
	if (ProjectileFinder.Succeeded())
	{
		ProjectileClass = ProjectileFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> CostEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_ManaCost"));
	if (CostEffectFinder.Succeeded())
	{
		CostGameplayEffectClass = CostEffectFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_Cooldown_Q"));
	if (CooldownEffectFinder.Succeeded())
	{
		CooldownGameplayEffectClass = CooldownEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> CastMontageFinder(TEXT("/Game/Characters/Animations/AM_CastFireball"));
	if (CastMontageFinder.Succeeded())
	{
		MontageToPlay = CastMontageFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_FireballDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		ProjectileData.DamageEffect = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> VisualEffectFinder(TEXT("/Game/FX/Niagara/NS_MyFireball"));
	if (VisualEffectFinder.Succeeded())
	{
		ProjectileData.VisualEffect = VisualEffectFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactEffectFinder(TEXT("/Game/FX/Niagara/NS_BurstSmoke"));
	if (ImpactEffectFinder.Succeeded())
	{
		ProjectileData.ImpactEffect = ImpactEffectFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> FlyingSoundFinder(TEXT("/Game/StarterContent/Audio/Fire_Sparks01_Cue"));
	if (FlyingSoundFinder.Succeeded())
	{
		ProjectileData.FlyingSound = FlyingSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSoundFinder(TEXT("/Game/StarterContent/Audio/Explosion_Cue"));
	if (ImpactSoundFinder.Succeeded())
	{
		ProjectileData.ImpactSound = ImpactSoundFinder.Object;
	}
	
	ProjectileData.MaxFlyDistance = 800.0f;
	ProjectileData.InitialSpeed = 1000.0f;
	ProjectileData.MaxSpeed = 1000.0f;
	ProjectileData.DamageAmount = 80.0f;
	ProjectileData.SplashRadius = 0.0f;
	ProjectileData.Lifespan = 2.0f;
	ProjectileData.bEnableHoming = true;
	ProjectileData.HomingAcceleration = 9000.0f;

	RangeToApply = 600.0f;
	
	SetUsesInteractiveTargeting(true);

	InputPolicy.InputMode = EAbilityInputMode::Instant;
	InputPolicy.bUpdateTargetWhileHeld = false;
	InputPolicy.bAllowInputBuffer = true;
	InputPolicy.MaxBufferWindow = 0.25f;
}
