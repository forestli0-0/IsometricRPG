#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GA_SelfCastAbility.h"
#include "GameplayEffect.h"
#include "GA_HealRegenAbility.generated.h"

class UGameplayEffect; // Forward declaration

/**
 * W技能：提升自身血量和蓝量的恢复速度，并增加移速
 */
UCLASS()
class ISOMETRICRPG_API UGA_HealRegenAbility : public UGA_SelfCastAbility
{
    GENERATED_BODY()
public:
    UGA_HealRegenAbility();
protected:
    // GameplayEffect to apply for the buff
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effect")
    TSubclassOf<UGameplayEffect> BuffGameplayEffectClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Cooldown")
    float ThisCooldownDuration = 1.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Cost")
    float ManaCost = 10.f;
    // 恢复加成百分比
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Buff")
    float HealthRegenBonusPercent = 100.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Buff")
    float ManaRegenBonusPercent = 100.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Buff")
    float MoveSpeedBonusPercent = 30.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Buff")
    float BuffDuration = 5.f;

    // 覆盖以应用自定义法力消耗
    virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

    // 覆盖以实现自定义消耗检查
    virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    // 执行自身施放
    virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
