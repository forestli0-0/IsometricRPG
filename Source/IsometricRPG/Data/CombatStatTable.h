#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "IsometricAbilities/Types/IsoDamageType.h"
#include "CombatStatTable.generated.h"

/**
 * Shared combat stat row for pawn/enemy data tables.
 */
USTRUCT(BlueprintType)
struct FCombatStatTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float MaxHealth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AbilityPower = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float PhysicalDefense = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float MagicDefense = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float PhysicalResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float FireResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float IceResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float LightningResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float ArmorPenetration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float MagicPenetration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float ElementalPenetration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float CriticalChance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float CriticalDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float BlockChance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float BlockDamageReduction = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float LifeSteal = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float ManaLeech = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    EIsoDamageType PreferredDamageType = EIsoDamageType::Physical;
};

