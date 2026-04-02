#include "IsometricAbilities/GameplayAbilities/HeroAbilityCommitHelper.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/HeroAbilityNotificationReceiver.h"

namespace
{
const FGameplayTag& GetCostMagnitudeTag()
{
    static const FGameplayTag CostMagnitudeTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cost.Magnitude"));
    return CostMagnitudeTag;
}
}

FString FHeroAbilityCommitHelper::DescribeTagContainer(const FGameplayTagContainer& Tags)
{
    TArray<FGameplayTag> TagArray;
    Tags.GetGameplayTagArray(TagArray);

    if (TagArray.Num() == 0)
    {
        return TEXT("(None)");
    }

    TArray<FString> TagStrings;
    TagStrings.Reserve(TagArray.Num());
    for (const FGameplayTag& Tag : TagArray)
    {
        TagStrings.Add(Tag.ToString());
    }

    return FString::Join(TagStrings, TEXT(", "));
}

FString FHeroAbilityCommitHelper::DescribeMatchedOwnedCooldownTags(
    const FGameplayTagContainer* CooldownTags,
    const FGameplayTagContainer& OwnedTags)
{
    if (!CooldownTags)
    {
        return TEXT("(None)");
    }

    TArray<FGameplayTag> CooldownTagArray;
    CooldownTags->GetGameplayTagArray(CooldownTagArray);

    TArray<FGameplayTag> OwnedTagArray;
    OwnedTags.GetGameplayTagArray(OwnedTagArray);

    TArray<FString> MatchedOwnedTagStrings;
    for (const FGameplayTag& OwnedTag : OwnedTagArray)
    {
        for (const FGameplayTag& CooldownTag : CooldownTagArray)
        {
            if (OwnedTag.MatchesTag(CooldownTag))
            {
                MatchedOwnedTagStrings.AddUnique(OwnedTag.ToString());
                break;
            }
        }
    }

    return MatchedOwnedTagStrings.Num() > 0
        ? FString::Join(MatchedOwnedTagStrings, TEXT(", "))
        : TEXT("(None)");
}

bool FHeroAbilityCommitHelper::HasMeaningfulCost(const float CostMagnitude)
{
    return !FMath::IsNearlyZero(CostMagnitude);
}

FGameplayEffectSpecHandle FHeroAbilityCommitHelper::MakeCostGameplayEffectSpecHandle(
    const UGameplayAbility& Ability,
    const FGameplayAbilitySpecHandle& Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo,
    TSubclassOf<UGameplayEffect> CostGameplayEffectClass,
    const float CostMagnitude,
    const float AbilityLevel)
{
    if (!HasMeaningfulCost(CostMagnitude) || !CostGameplayEffectClass)
    {
        return FGameplayEffectSpecHandle();
    }

    FGameplayEffectSpecHandle CostSpecHandle = Ability.MakeOutgoingGameplayEffectSpec(
        Handle,
        ActorInfo,
        ActivationInfo,
        CostGameplayEffectClass,
        AbilityLevel);
    if (!CostSpecHandle.Data.IsValid())
    {
        return FGameplayEffectSpecHandle();
    }

    FGameplayEffectSpec& CostSpec = *CostSpecHandle.Data.Get();
    CostSpec.SetSetByCallerMagnitude(GetCostMagnitudeTag(), -CostMagnitude);
    CostSpec.CalculateModifierMagnitudes();
    return CostSpecHandle;
}

float FHeroAbilityCommitHelper::ResolveManaCostFromSpec(
    const UGameplayAbility& Ability,
    const FGameplayEffectSpec& CostSpec)
{
    const FGameplayAttribute ManaAttribute = UIsometricRPGAttributeSetBase::GetManaAttribute();
    float ResolvedManaCost = 0.f;

    const UGameplayEffect* CostDefinition = CostSpec.Def;
    if (!CostDefinition)
    {
        return ResolvedManaCost;
    }

    const TArray<FGameplayModifierInfo>& Modifiers = CostDefinition->Modifiers;
    for (int32 ModifierIndex = 0; ModifierIndex < Modifiers.Num(); ++ModifierIndex)
    {
        const FGameplayModifierInfo& Modifier = Modifiers[ModifierIndex];
        if (Modifier.Attribute != ManaAttribute)
        {
            continue;
        }

        if (Modifier.ModifierOp != EGameplayModOp::Additive)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("%s: Cost GE %s uses a non-additive mana modifier. Current resolver ignores this path."),
                *Ability.GetName(),
                *GetNameSafe(CostDefinition));
            continue;
        }

        const float ModifierMagnitude = CostSpec.GetModifierMagnitude(ModifierIndex, true);
        if (ModifierMagnitude < 0.f)
        {
            ResolvedManaCost += -ModifierMagnitude;
        }
    }

    return ResolvedManaCost;
}

bool FHeroAbilityCommitHelper::CheckCost(
    const UGameplayAbility& Ability,
    const FGameplayAbilitySpecHandle& Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    TSubclassOf<UGameplayEffect> CostGameplayEffectClass,
    const float CostMagnitude,
    FGameplayTagContainer* OptionalRelevantTags)
{
    if (!HasMeaningfulCost(CostMagnitude))
    {
        return true;
    }

    UAbilitySystemComponent* ASC = Ability.GetAbilitySystemComponentFromActorInfo();
    if (!ASC && ActorInfo)
    {
        ASC = ActorInfo->AbilitySystemComponent.Get();
    }

    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AbilitySystemComponent is null in CheckCost."), *Ability.GetName());
        return false;
    }

    const FGameplayEffectSpecHandle CostSpecHandle = MakeCostGameplayEffectSpecHandle(
        Ability,
        Handle,
        ActorInfo,
        FGameplayAbilityActivationInfo(),
        CostGameplayEffectClass,
        CostMagnitude,
        Ability.GetAbilityLevel(Handle, ActorInfo));
    if (!CostSpecHandle.Data.IsValid())
    {
        if (!CostGameplayEffectClass)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: CostGameplayEffectClass is not configured."), *Ability.GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Failed to build cost GE spec from %s."), *Ability.GetName(), *GetNameSafe(CostGameplayEffectClass));
        }

        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailCostTag);
        }
        return false;
    }

    const UIsometricRPGAttributeSetBase* LocalAttributeSet = ASC->GetSet<UIsometricRPGAttributeSetBase>();
    if (!LocalAttributeSet)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AttributeSet is null in CheckCost."), *Ability.GetName());
        return false;
    }

    const float ResolvedManaCost = ResolveManaCostFromSpec(Ability, *CostSpecHandle.Data.Get());
    if (LocalAttributeSet->GetMana() < ResolvedManaCost)
    {
        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailCostTag);
        }
        return false;
    }

    return true;
}

void FHeroAbilityCommitHelper::ApplyCost(
    const UGameplayAbility& Ability,
    const FGameplayAbilitySpecHandle& Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo,
    TSubclassOf<UGameplayEffect> CostGameplayEffectClass,
    const float CostMagnitude,
    const float AbilityLevel)
{
    if (!HasMeaningfulCost(CostMagnitude))
    {
        return;
    }

    UAbilitySystemComponent* ASC = Ability.GetAbilitySystemComponentFromActorInfo();
    if (!ASC && ActorInfo)
    {
        ASC = ActorInfo->AbilitySystemComponent.Get();
    }

    const FGameplayEffectSpecHandle CostSpecHandle = MakeCostGameplayEffectSpecHandle(
        Ability,
        Handle,
        ActorInfo,
        ActivationInfo,
        CostGameplayEffectClass,
        CostMagnitude,
        AbilityLevel);
    if (ASC && CostSpecHandle.Data.IsValid())
    {
        ASC->ApplyGameplayEffectSpecToSelf(*CostSpecHandle.Data.Get(), ActivationInfo.GetActivationPredictionKey());
        return;
    }

    if (!CostGameplayEffectClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: CostGameplayEffectClass is not configured."), *Ability.GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to apply cost because cost GE spec could not be built from %s."), *Ability.GetName(), *GetNameSafe(CostGameplayEffectClass));
    }
}

bool FHeroAbilityCommitHelper::ApplyCooldown(
    const UGameplayAbility& Ability,
    const FGameplayAbilityActorInfo* ActorInfo,
    TSubclassOf<UGameplayEffect> CooldownGameplayEffectClass,
    const float CooldownDuration,
    const float AbilityLevel)
{
    if (!CooldownGameplayEffectClass || !ActorInfo)
    {
        return false;
    }

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
    {
        return false;
    }

    FGameplayEffectSpecHandle SpecHandle = Ability.MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, AbilityLevel);
    if (!SpecHandle.Data.IsValid())
    {
        return false;
    }

    FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();
    const FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration"));
    if (CooldownDurationTag.IsValid())
    {
        GESpec.SetSetByCallerMagnitude(CooldownDurationTag, CooldownDuration);
    }

    ASC->ApplyGameplayEffectSpecToSelf(GESpec);
    return true;
}

bool FHeroAbilityCommitHelper::ApplyAttackSpeedCooldown(
    const UGameplayAbility& Ability,
    const FGameplayAbilityActorInfo* ActorInfo,
    TSubclassOf<UGameplayEffect> CooldownGameplayEffectClass,
    const UIsometricRPGAttributeSetBase* AttributeSet,
    const float AbilityLevel,
    float& OutAppliedCooldownDuration)
{
    OutAppliedCooldownDuration = 0.0f;

    if (!CooldownGameplayEffectClass || !AttributeSet || !ActorInfo)
    {
        return false;
    }

    UAbilitySystemComponent* ASC = Ability.GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        ASC = ActorInfo->AbilitySystemComponent.Get();
    }

    if (!ASC)
    {
        return false;
    }

    FGameplayEffectSpecHandle SpecHandle = Ability.MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, AbilityLevel);
    if (!SpecHandle.Data.IsValid())
    {
        return false;
    }

    const float SafeAttackSpeed = AttributeSet->GetAttackSpeed() > 0.0f ? AttributeSet->GetAttackSpeed() : 1.0f;
    OutAppliedCooldownDuration = 1.0f / SafeAttackSpeed;

    FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();
    const FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration"));
    GESpec.SetSetByCallerMagnitude(CooldownDurationTag, OutAppliedCooldownDuration);
    ASC->ApplyGameplayEffectSpecToSelf(GESpec);
    return true;
}

void FHeroAbilityCommitHelper::NotifyCooldownTriggered(
    const FGameplayAbilitySpecHandle& Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const float CooldownDuration)
{
    if (!ActorInfo || CooldownDuration <= 0.f)
    {
        return;
    }

    UObject* NotificationReceiver = nullptr;

    if (AController* Controller = Cast<AController>(ActorInfo->PlayerController.Get()))
    {
        NotificationReceiver = Cast<UObject>(Controller);
    }

    if (const APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
    {
        if (!NotificationReceiver)
        {
            NotificationReceiver = Cast<UObject>(Pawn->GetController());
        }
    }

    if (NotificationReceiver && NotificationReceiver->GetClass()->ImplementsInterface(UHeroAbilityNotificationReceiver::StaticClass()))
    {
        IHeroAbilityNotificationReceiver::Execute_NotifyAbilityCooldownTriggered(NotificationReceiver, Handle, CooldownDuration);
    }
}
