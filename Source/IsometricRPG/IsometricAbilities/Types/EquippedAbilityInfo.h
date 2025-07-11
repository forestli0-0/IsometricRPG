#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "EquippedAbilityInfo.generated.h"

UENUM(BlueprintType)
enum class ESkillSlot : uint8
{
    Invalid UMETA(Hidden),
	Confirm UMETA(DisplayName = "Confirm"),
    RightClick UMETA(DisplayName = "Right Click"),
    Skill_A UMETA(DisplayName = "Skill A"),
    Skill_Q UMETA(DisplayName = "Skill Q"),
    Skill_W UMETA(DisplayName = "Skill W"),
    Skill_E UMETA(DisplayName = "Skill E"),
    Skill_R UMETA(DisplayName = "Skill R"),
    Skill_Passive UMETA(DisplayName="Skill Passive"),
    Skill_Summoner1 UMETA(DisplayName="Skill Summoner 1"),
    Skill_Summoner2 UMETA(DisplayName="Skill Summoner 2"),
    Skill_Death UMETA(DisplayName = "Skill Death"),
	MAX UMETA(Hidden) // Keep this as the last entry to ensure the enum is always valid
};

USTRUCT(BlueprintType)
struct FEquippedAbilityInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
    TSubclassOf<class UGameplayAbility> AbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
    ESkillSlot Slot = ESkillSlot::Invalid;
    
    UPROPERTY(BlueprintReadOnly, Category="Ability") 
    FGameplayAbilitySpecHandle AbilitySpecHandle; 

    FEquippedAbilityInfo() : AbilityClass(nullptr), Slot(ESkillSlot::Invalid) {}
    FEquippedAbilityInfo(TSubclassOf<class UGameplayAbility> InAbility, ESkillSlot InSlot) 
        : AbilityClass(InAbility), Slot(InSlot) {}
};
