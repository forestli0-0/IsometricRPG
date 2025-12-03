#include "IsometricAIController.h"
#include "Perception/AIPerceptionComponent.h"

AIsometricAIController::AIsometricAIController()
{
	// Create the perception component
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	
    // Tell the AIController to use this component
    SetPerceptionComponent(*AIPerceptionComponent);
}
