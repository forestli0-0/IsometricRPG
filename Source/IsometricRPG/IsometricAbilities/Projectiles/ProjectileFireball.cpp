// 在项目设置的描述页填写版权声明。


#include "IsometricAbilities/Projectiles/ProjectileFireball.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include <Kismet/KismetSystemLibrary.h>
AProjectileFireball::AProjectileFireball()
{

}
void AProjectileFireball::ApplyDamageEffects(AActor* TargetActor, const FHitResult& HitResult)
{
	Super::ApplyDamageEffects(TargetActor, HitResult);
	auto TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    if (BurnEffect)
    {
        if (SourceAbilitySystemComponent == nullptr || SourceAbility == nullptr)
        {
            return;
		}
        FGameplayEffectContextHandle BurnContextHandle = SourceAbilitySystemComponent->MakeEffectContext();
        BurnContextHandle.AddSourceObject(this);
        FGameplayEffectSpecHandle BurnSpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(BurnEffect, SourceAbility->GetAbilityLevel(), BurnContextHandle);
        if (BurnSpecHandle.IsValid())
        {
            BurnSpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.PerTick")), -BurnDamagePerTick);
            // 应用灼烧效果
            SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*BurnSpecHandle.Data.Get(), TargetASC);
        }
    }
}
void AProjectileFireball::HandleSplashDamage(const FVector& ImpactLocation, AActor* DirectHitActor)
{
    if (SourceAbilitySystemComponent && InitData.SplashDamageEffect && InitData.SplashRadius > 0.0f)
    {
        TArray<AActor*> OverlappedActors;
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)); // 可以根据需要配置检测的物体类型
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Destructible));

        UKismetSystemLibrary::SphereOverlapActors(
            GetWorld(),
            ImpactLocation,
            InitData.SplashRadius,
            ObjectTypes,
            nullptr, // 过滤的 Actor 类（可选）
            TArray<AActor*>({ ProjectileOwner, DirectHitActor }), // 忽略的 Actor（发射者与直接命中目标）
            OverlappedActors
        );

        for (AActor* OverlappedActor : OverlappedActors)
        {
            if (OverlappedActor)
            {
                UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OverlappedActor);
                if (TargetASC)
                {
                    FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();
                    EffectContext.AddSourceObject(this);
                    EffectContext.AddInstigator(ProjectileInstigator, ProjectileOwner);
                    // 对于溅射，HitResult可能不直接相关，或者可以创建一个新的HitResult指向溅射中心

                    FGameplayEffectSpecHandle DirectSpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(InitData.SplashDamageEffect, 1.0f, EffectContext);
                    float SplashDamage = -InitData.DamageAmount * 0.3;
                    DirectSpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), SplashDamage);
                    if (DirectSpecHandle.IsValid())
                    {
                        SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*DirectSpecHandle.Data.Get(), TargetASC);
                    }

                    if (BurnEffect)
                    {
                        FGameplayEffectContextHandle BurnContextHandle = SourceAbilitySystemComponent->MakeEffectContext();
                        BurnContextHandle.AddSourceObject(this);
                        FGameplayEffectSpecHandle BurnSpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(BurnEffect, SourceAbility->GetAbilityLevel(), BurnContextHandle);
                        if (BurnSpecHandle.IsValid())
                        {
                            BurnSpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.PerTick")), -BurnDamagePerTick);
                            // 应用灼烧效果
                            SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*BurnSpecHandle.Data.Get(), TargetASC);
                        }
                    }
                }
            }
        }
    }
}
