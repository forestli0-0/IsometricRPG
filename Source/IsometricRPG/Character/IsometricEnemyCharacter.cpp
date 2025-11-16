#include "IsometricEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "IsometricRPGAttributeSetBase.h"

AIsometricEnemyCharacter::AIsometricEnemyCharacter()
{
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    AttributeSet = CreateDefaultSubobject<UIsometricRPGAttributeSetBase>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AIsometricEnemyCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UIsometricRPGAttributeSetBase* AIsometricEnemyCharacter::GetAttributeSet() const
{
    return AttributeSet;
}

void AIsometricEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }

    if (HasAuthority())
    {
        // 初始化属性
        if (AbilitySystemComponent && DefaultAttributesEffect)
        {
            FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
            EffectContext.AddSourceObject(this);
            FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1, EffectContext);
            if (SpecHandle.IsValid())
            {
                AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            }
        }

        // 授予默认技能
        if (AbilitySystemComponent)
        {
            for (auto& AbilityClass : DefaultAbilities)
            {
                if (AbilityClass)
                {
                    AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
                }
            }
        }
    }
}
