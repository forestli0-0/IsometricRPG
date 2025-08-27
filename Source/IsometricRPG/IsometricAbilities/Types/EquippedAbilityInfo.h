#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "EquippedAbilityInfo.generated.h"

UENUM(BlueprintType)
enum class ESkillSlot : uint8
{
    Invalid UMETA(Hidden),
    Skill_Passive UMETA(DisplayName="Skill Passive"),
    Skill_Q UMETA(DisplayName = "Skill Q"),
    Skill_W UMETA(DisplayName = "Skill W"),
    Skill_E UMETA(DisplayName = "Skill E"),
    Skill_R UMETA(DisplayName = "Skill R"),
    Skill_D UMETA(DisplayName="Skill Summoner 1"),
    Skill_F UMETA(DisplayName="Skill Summoner 2"),
	MAX UMETA(Hidden) // Keep this as the last entry to ensure the enum is always valid
};

USTRUCT(BlueprintType)
struct FEquippedAbilityInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
    TSoftClassPtr<class UGameplayAbility> AbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
    ESkillSlot Slot = ESkillSlot::Invalid;
    
    UPROPERTY(BlueprintReadOnly, Category="Ability") 
    FGameplayAbilitySpecHandle AbilitySpecHandle; 

    // 技能图标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    TSoftObjectPtr<UTexture2D> Icon;

    FEquippedAbilityInfo() : AbilityClass(nullptr), Slot(ESkillSlot::Invalid) {}
    FEquippedAbilityInfo(TSubclassOf<class UGameplayAbility> InAbility, ESkillSlot InSlot) 
        : AbilityClass(InAbility), Slot(InSlot) {}
};
