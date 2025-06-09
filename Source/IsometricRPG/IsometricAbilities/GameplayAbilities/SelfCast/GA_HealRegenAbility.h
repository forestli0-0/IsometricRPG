// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_HealRegenAbility.h
#pragma once

#include "CoreMinimal.h"
#include "GA_SelfCastAbility.h"
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
    TSubclassOf<UGameplayEffect> HealGameplayEffectClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effect")
    TSubclassOf<UGameplayEffect> SpeedGameplayEffectClass;


    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Buff")
    float HealPercent = 1.2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Buff")
    float MoveSpeedBonusPercent = 0.3f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Buff")
    float BuffDuration = 5.f;
    virtual void ApplySelfEffect(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
};
