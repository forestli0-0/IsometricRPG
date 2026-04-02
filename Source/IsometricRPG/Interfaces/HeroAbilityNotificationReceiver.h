#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HeroAbilityNotificationReceiver.generated.h"

struct FGameplayAbilitySpecHandle;

UINTERFACE(BlueprintType)
class ISOMETRICRPG_API UHeroAbilityNotificationReceiver : public UInterface
{
    GENERATED_BODY()
};

class ISOMETRICRPG_API IHeroAbilityNotificationReceiver
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, Category = "Ability|Notifications")
    void NotifyAbilityCooldownTriggered(const FGameplayAbilitySpecHandle& SpecHandle, float DurationSeconds);
};
