#include "GA_Ahri_Q_OrbOfDeception.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

UGA_Ahri_Q_OrbOfDeception::UGA_Ahri_Q_OrbOfDeception()
{
	// 设置技能基础属性
	AbilityType = EHeroAbilityType::SkillShot;
	CooldownDuration = 7.0f;
	CostMagnitude = 65.0f;
	RangeToApply = 880.0f;
	
	// 设置方向技能属性
	SkillShotWidth = 80.0f;

	// 为这个技能配置投射物数据
	ProjectileData.InitialSpeed = 1600.f;
	ProjectileData.MaxSpeed = ProjectileData.InitialSpeed;
	ProjectileData.MaxFlyDistance = RangeToApply;
	ProjectileData.SplashRadius = SkillShotWidth;

	// 往返
	ProjectileData.bReturnToOwner = true;
	ProjectileData.ReturnSpeedMultiplier = 1.0f;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_AttackDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		ProjectileData.DamageEffect = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> OrbVFXFinder(TEXT("/Game/FX/Niagara/NS_MyFireball"));
	if (OrbVFXFinder.Succeeded())
	{
		OrbVFX = OrbVFXFinder.Object;
		ProjectileData.VisualEffect = OrbVFX;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitVFXFinder(TEXT("/Game/FX/Niagara/NS_BurstSmoke"));
	if (HitVFXFinder.Succeeded())
	{
		HitVFX = HitVFXFinder.Object;
		ProjectileData.ImpactEffect = HitVFX;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> HitSoundFinder(TEXT("/Game/StarterContent/Audio/Explosion_Cue"));
	if (HitSoundFinder.Succeeded())
	{
		HitSound = HitSoundFinder.Object;
		ProjectileData.ImpactSound = HitSound;
	}
}

void UGA_Ahri_Q_OrbOfDeception::ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation)
{
	ProjectileData.DamageAmount = CalculateDamage();
	ProjectileData.VisualEffect = OrbVFX;
	ProjectileData.ImpactEffect = HitVFX;
	ProjectileData.ImpactSound = HitSound;
	Super::ExecuteSkillShot(Direction, StartLocation);
}

float UGA_Ahri_Q_OrbOfDeception::CalculateDamage() const
{
	if (!AttributeSet)
	{
		// 尝试从ActorInfo获取属性集
		const UIsometricRPGAttributeSetBase* RPGAttributeSet = GetAbilitySystemComponentFromActorInfo()->GetSet<UIsometricRPGAttributeSetBase>();
		if (!RPGAttributeSet) return 0.0f;
		
		// 获取技能等级
		const int32 AbilityLevel = GetAbilityLevel();
	
		// 获取法术强度
		const float AbilityPower = RPGAttributeSet->GetAttackDamage();

		// 计算基础伤害
		float TotalDamage = BaseMagicDamage + (DamagePerLevel * (AbilityLevel - 1));
	
		// 添加法术强度加成
		TotalDamage += AbilityPower * APRatio;

		return TotalDamage;
	}
	
	// 如果AttributeSet已经存在
	const int32 AbilityLevel = GetAbilityLevel();
	const float AbilityPower = AttributeSet->GetAttackDamage();
	float TotalDamage = BaseMagicDamage + (DamagePerLevel * (AbilityLevel - 1));
	TotalDamage += AbilityPower * APRatio;

	return TotalDamage;
}

#if WITH_EDITOR
EDataValidationResult UGA_Ahri_Q_OrbOfDeception::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return Result;
	}

	const TSubclassOf<AProjectileBase> ConfiguredProjectileClass = GetConfiguredProjectileClass();
	const UClass* ConfiguredProjectileUClass = ConfiguredProjectileClass.Get();
	const FString ExpectedProjectileClassPath = TEXT("/Game/Blueprints/Projectiles/BP_Projectile_AhriQ.BP_Projectile_AhriQ_C");
	if (!ConfiguredProjectileUClass || ConfiguredProjectileUClass->GetPathName() != ExpectedProjectileClassPath)
	{
		Context.AddError(FText::Format(
			NSLOCTEXT("AhriOrbOfDeception", "WrongProjectileClass", "{0}: Ahri Q must use BP_Projectile_AhriQ as its projectile class."),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}

	const UGA_Ahri_Q_OrbOfDeception* SuperDefaults = Cast<UGA_Ahri_Q_OrbOfDeception>(GetClass()->GetSuperClass()->GetDefaultObject());
	if (!SuperDefaults)
	{
		return Result;
	}

	const auto AddLockedFieldError = [&](const TCHAR* FieldName)
	{
		Context.AddError(FText::Format(
			NSLOCTEXT("AhriOrbOfDeception", "LockedProjectileDataOverride", "{0}: ProjectileData.{1} should stay on the Ahri Q native defaults."),
			FText::FromString(GetName()),
			FText::FromString(FieldName)));
		Result = EDataValidationResult::Invalid;
	};

	if (!FMath::IsNearlyEqual(ProjectileData.MaxFlyDistance, SuperDefaults->ProjectileData.MaxFlyDistance))
	{
		AddLockedFieldError(TEXT("MaxFlyDistance"));
	}

	if (!FMath::IsNearlyEqual(ProjectileData.Lifespan, SuperDefaults->ProjectileData.Lifespan))
	{
		AddLockedFieldError(TEXT("Lifespan"));
	}

	if (ProjectileData.DamageEffect != SuperDefaults->ProjectileData.DamageEffect)
	{
		AddLockedFieldError(TEXT("DamageEffect"));
	}

	if (ProjectileData.SplashDamageEffect != SuperDefaults->ProjectileData.SplashDamageEffect)
	{
		AddLockedFieldError(TEXT("SplashDamageEffect"));
	}

	if (ProjectileData.VisualEffect != SuperDefaults->ProjectileData.VisualEffect)
	{
		AddLockedFieldError(TEXT("VisualEffect"));
	}

	return Result;
}
#endif
