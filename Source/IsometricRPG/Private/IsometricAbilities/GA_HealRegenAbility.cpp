#include "IsometricAbilities/GA_HealRegenAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_HealRegenAbility::UGA_HealRegenAbility()
{
    //SkillType = EHeroSkillType::SelfCast;
    //// 设置技能的GameplayTag
    //AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.W"));
    //ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.W"));

    //// 设置技能触发事件
    //FAbilityTriggerData TriggerData;
    //TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Ability.W");
    //TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    //AbilityTriggers.Add(TriggerData);

    //// 设定冷却阻断标签
    //ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Cooldown.Ability.W"));

    //static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_Cooldown"));
    //if (CooldownEffectObj.Succeeded())
    //{
    //    CooldownGameplayEffectClass = CooldownEffectObj.Class;
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Error, TEXT("Failed to load CooldownEffect! Check path validity."));
    //    // 可考虑添加调试断言
    //    checkNoEntry();
    //}
    //CooldownDuration = ThisCooldownDuration; // 设置冷却时间
    //// 设置技能消耗效果类
    //static ConstructorHelpers::FClassFinder<UGameplayEffect> CostEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_ManaCost"));
    //if (CostEffectObj.Succeeded())
    //{
    //    CostGameplayEffectClass = CostEffectObj.Class;
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Error, TEXT("Failed to load CostGameplayEffect! Check path validity."));
    //    // 可考虑添加调试断言
    //    checkNoEntry();
    //}
}
// 实现 ApplyCost 方法以应用自定义法力消耗
void UGA_HealRegenAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{    // 检查蓝是否足够
    //if (AttributeSet && AttributeSet->GetMana() < ManaCost)
    //{
    //    // 蓝不足时，不应用消耗GE，也不调用父类
    //    UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::ApplyCost - Not enough mana, skipping cost application."));
    //    return;
    //}
    //if (CostGameplayEffectClass)
    //{
    //    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    //    {
    //        UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
    //        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CostGameplayEffectClass, GetAbilityLevel());

    //        if (SpecHandle.Data.IsValid())
    //        {
    //            FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();

    //            // 这个 GameplayTag 必须与你的 CostGameplayEffectClass (例如 GE_ManaCost) 中的
    //            // SetByCaller_Magnitude 相匹配，该 GE 用于减少法力值。
    //            FGameplayTag ManaMagnitudeTag = FGameplayTag::RequestGameplayTag(FName("Data.Cost.Mana"));

    //            // 设置法力消耗的量。确保 ManaCost 是正值。
    //            GESpec.SetSetByCallerMagnitude(ManaMagnitudeTag, -ManaCost);

    //            UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    //            ASC->ApplyGameplayEffectSpecToSelf(GESpec);
    //        }
    //    }
    //    else
    //    {
    //        UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::ApplyCost - ActorInfo or ASC is invalid."));
    //    }
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::ApplyCost - CostGameplayEffectClass is not set. Calling Super::ApplyCost."));
    //    Super::ApplyCost(Handle, ActorInfo, ActivationInfo); // 如果没有指定消耗GE，则调用基类实现
    //}
    //// 应用冷却效果
    //if (CooldownGameplayEffectClass)
    //{
    //    FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
    //    if (SpecHandle.Data.IsValid())
    //    {
    //        FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();

    //        // 获取你在 GE 中配置的 Data Tag
    //        FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration"));

    //        // 设置 Set by Caller 的值
    //        GESpec.SetSetByCallerMagnitude(CooldownDurationTag, CooldownDuration);

    //        UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    //        ASC->ApplyGameplayEffectSpecToSelf(GESpec);
    //    }
    //}
}

// 覆盖以实现自定义消耗检查
bool UGA_HealRegenAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    //// 首先调用父类的CheckCost，确保其他潜在的消耗检查（如冷却）通过
    //if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
    //{
    //    return false;
    //}

    //// 确保ActorInfo和AbilitySystemComponent有效
    //if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::CheckCost - ActorInfo or ASC is invalid."));
    //    return false;
    //}

    //const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

    //// 获取自定义属性集
    //const UIsometricRPGAttributeSetBase* MyAttributeSet = Cast<const UIsometricRPGAttributeSetBase>(ASC->GetAttributeSet(UIsometricRPGAttributeSetBase::StaticClass()));

    //if (!MyAttributeSet)
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::CheckCost - UIsometricRPGAttributeSetBase not found on ASC. Cannot check mana cost."));
    //    // 根据游戏逻辑，如果找不到属性集，可能意味着无法执行消耗检查
    //    // 这里我们严格处理，如果无法检查则视为失败
    //    if (OptionalRelevantTags)
    //    {
    //        OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Failed.InternalError.MissingAttributes")));
    //    }
    //    return false;
    //}

    //// 获取当前法力值 (假设您的属性集有 GetMana() 方法并且是const的)
    //float CurrentMana = MyAttributeSet->GetMana();

    //// 检查法力是否足够
    //// ManaCost 是 UGA_FireballAbility 的成员变量
    //if (CurrentMana < ManaCost)
    //{
    //    if (OptionalRelevantTags)
    //    {
    //        // 添加一个通用的消耗失败标签和更具体的法力不足标签
    //        OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Failed.Cost")));
    //        OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Failed.NotEnoughMana")));
    //    }
    //    // 主动触发事件，通知ASC和动作队列（ActionQueueComponent）
    //    FGameplayEventData FailEventData;
    //    FailEventData.EventTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failed.Cost"));
    //    FailEventData.Instigator = GetAvatarActorFromActorInfo();
    //    FailEventData.Target = GetAvatarActorFromActorInfo();
    //    UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
    //    if (MyASC)
    //    {
    //        MyASC->HandleGameplayEvent(FailEventData.EventTag, &FailEventData);
    //    }
    //    UE_LOG(LogTemp, Log, TEXT("%s CheckCost failed due to insufficient mana. Required: %.2f, Current: %.2f"), *GetName(), ManaCost, CurrentMana);
    //    return false;
    //}

    //// 如果所有检查都通过
    return true;
}
void UGA_HealRegenAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    //Super::ExecuteSelfCast();

    //UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    //if (!ASC)
    //{
    //    UE_LOG(LogTemp, Error, TEXT("UGA_HealRegenAbility: Cannot get AbilitySystemComponent."));
    //    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
    //    return;
    //}

    //if (!IsValid(BuffGameplayEffectClass))
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("UGA_HealRegenAbility: BuffGameplayEffectClass is not set for ability %s. Ensure it is assigned in the Blueprint."), *GetName());
    //    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
    //    return;
    //}

    //FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
    //EffectContext.AddSourceObject(this);

    //FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BuffGameplayEffectClass, GetAbilityLevel(), EffectContext);

    //if (SpecHandle.IsValid())
    //{
    //    FActiveGameplayEffectHandle ActiveGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    //    if (!ActiveGEHandle.WasSuccessfullyApplied())
    //    {
    //        UE_LOG(LogTemp, Warning, TEXT("UGA_HealRegenAbility: Failed to apply buff GameplayEffect %s."), *GetNameSafe(BuffGameplayEffectClass))
    //    }
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("UGA_HealRegenAbility: Failed to make spec for GameplayEffect %s."), *GetNameSafe(BuffGameplayEffectClass));
    //}

    //EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

