// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "HeroAbilityTypes.generated.h"

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

