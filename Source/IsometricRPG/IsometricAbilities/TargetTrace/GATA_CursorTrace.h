// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "GATA_CursorTrace.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API AGATA_CursorTrace : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
    AGATA_CursorTrace();

    /** The trace channel to use for the cursor trace. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Targeting")
    TEnumAsByte<ETraceTypeQuery> TraceChannel;

    /** Whether the trace should be complex, enabling tracing against individual triangles of complex meshes. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Targeting")
    bool bTraceComplex;

    //~ Begin AGameplayAbilityTargetActor Interface
    virtual void StartTargeting(UGameplayAbility* Ability) override;
    virtual void ConfirmTargetingAndContinue() override;
    //~ End AGameplayAbilityTargetActor Interface
	
	
};
