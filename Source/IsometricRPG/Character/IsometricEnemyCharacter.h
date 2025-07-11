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

    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UIsometricRPGAttributeSetBase> AttributeSet;
};
