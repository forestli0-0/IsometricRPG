#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "HeroAbilityTypes.generated.h"

UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None            UMETA(DisplayName = "None"),
	Confirm         UMETA(DisplayName = "Confirm"),
	Cancel          UMETA(DisplayName = "Cancel"),
	// --- Skills ---
	Ability_A          UMETA(DisplayName = "Attack (A)"),
	Ability_Q          UMETA(DisplayName = "Skill 1 (Q)"),
	Ability_W          UMETA(DisplayName = "Skill 2 (W)"),
	Ability_E          UMETA(DisplayName = "Skill 3 (E)"),
	Ability_R UMETA(DisplayName = "Skill 4 (R)"),
	Ability_Summoner1       UMETA(DisplayName = "Summoner 1"),
	Ability_Summoner2       UMETA(DisplayName = "Summoner 2")
};

UENUM(BlueprintType)
enum class EHeroAbilitySlot : uint8
{
    None        UMETA(DisplayName = "None"),
    Q           UMETA(DisplayName = "Q"),
    W           UMETA(DisplayName = "W"),
    E           UMETA(DisplayName = "E"),
	R           UMETA(DisplayName = "R"),
    Passive     UMETA(DisplayName = "Passive"),
    Summoner1   UMETA(DisplayName = "Summoner1"),
    Summoner2   UMETA(DisplayName = "Summoner2"),
    Max         UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FHeroAbilitySlotData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHeroAbilitySlot Slot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UGameplayAbility> AbilityClass;
};
