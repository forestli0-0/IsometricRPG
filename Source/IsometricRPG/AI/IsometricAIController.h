#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "IsometricAIController.generated.h"

/**
 * Base AI Controller class for IsometricRPG.
 * Contains stable components like AIPerception to avoid Blueprint data loss during TS recompilation.
 */
UCLASS()
class ISOMETRICRPG_API AIsometricAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AIsometricAIController();

protected:
	// AI Perception Component - Defined in C++ for stability
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<class UAIPerceptionComponent> AIPerceptionComponent;
};
