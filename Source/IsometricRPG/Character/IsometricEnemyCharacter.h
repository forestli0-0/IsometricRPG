#pragma once
#include "CoreMinimal.h"
#include "IsometricRPGCharacter.h"
#include "IsometricEnemyCharacter.generated.h"

UCLASS()
class ISOMETRICRPG_API AIsometricEnemyCharacter : public AIsometricRPGCharacter
{
    GENERATED_BODY()
public:
    AIsometricEnemyCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    virtual UIsometricRPGAttributeSetBase* GetAttributeSet() const override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TSubclassOf<class UGameplayEffect> DefaultAttributesEffect;

    // 修改：添加 BlueprintReadOnly 以便 TS/蓝图可以读取配置的技能列表
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UIsometricRPGAttributeSetBase> AttributeSet;
};
