#include "GA_Ahri_Q_OrbOfDeception.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

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
	// 这些数据将被传递给基类生成的投射物
	ProjectileData.InitialSpeed = 1600.f;
	ProjectileData.MaxFlyDistance = RangeToApply; // 直接使用基类的射程
	ProjectileData.SplashRadius = SkillShotWidth; // 使用技能宽度作为碰撞半径
	// 注意：阿狸的Q技能有独特的返回机制，这需要在投射物类本身 (蓝图或C++) 中实现。
	// 这个C++技能类只负责提供初始数据。
	// ProjectileData.VisualEffect = OrbVFX; // 这些应该在蓝图中设置
	// ProjectileData.ImpactEffect = HitVFX;
}

void UGA_Ahri_Q_OrbOfDeception::ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation)
{
	// 这个函数体现在应该是空的。
	// 所有的投射物生成和初始化逻辑都在基类 UGA_SkillShotProjectileAbility::ExecuteSkillShot 中处理。
	// 我们在这里只需要调用父类的实现即可。
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

// 所有旧的 OnOrb... 函数都已删除，因为投射物现在是自包含的。 