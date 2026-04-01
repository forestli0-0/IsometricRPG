#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/Function.h"

#include "IsometricAbilities/Types/EquippedAbilityInfo.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UHUDRootWidget;
class UIsometricRPGAttributeSetBase;
class UTexture2D;

struct FHUDPresentationContext
{
    const UIsometricRPGAttributeSetBase* AttributeSet = nullptr;
    const UAbilitySystemComponent* AbilitySystemComponent = nullptr;
    const TArray<FEquippedAbilityInfo>* EquippedAbilities = nullptr;
    const TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>>* BuffIconMap = nullptr;
    UWorld* World = nullptr;
    bool bShowOwnedTagsOnHUD = false;
};

class ISOMETRICRPG_API FHUDPresentationBuilder
{
public:
    static void RefreshActionBar(
        UHUDRootWidget& HUD,
        const TArray<FEquippedAbilityInfo>& EquippedAbilities,
        TFunctionRef<bool(const UGameplayAbility*, float&, float&)> QueryCooldownState);

    static void RefreshVitals(UHUDRootWidget& HUD, const UIsometricRPGAttributeSetBase* AttributeSet);

    static void RefreshChampionStats(UHUDRootWidget& HUD, const UIsometricRPGAttributeSetBase* AttributeSet);

    static void RefreshGameplayTagPresentation(UHUDRootWidget& HUD, const FHUDPresentationContext& Context);

    static void RefreshExperience(UHUDRootWidget& HUD, const UIsometricRPGAttributeSetBase* AttributeSet);

    static void RefreshUtilityButtons(UHUDRootWidget& HUD);

    static void RefreshEntireHUD(
        UHUDRootWidget& HUD,
        const FHUDPresentationContext& Context,
        TFunctionRef<bool(const UGameplayAbility*, float&, float&)> QueryCooldownState);
};
