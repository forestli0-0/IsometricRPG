#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsometricPathFollowingComponent.generated.h"

class ACharacter;

UENUM(BlueprintType)
enum class EIsometricPathFollowResult : uint8
{
	Succeeded,
	Failed,
	Aborted
};

DECLARE_MULTICAST_DELEGATE_OneParam(FIsometricPathFollowResultDelegate, EIsometricPathFollowResult);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ISOMETRICRPG_API UIsometricPathFollowingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsometricPathFollowingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool RequestMoveToLocation(const FVector& InTargetLocation, float InAcceptanceRadius = 35.0f);
	bool RequestMoveToActor(AActor* InTargetActor, float InAcceptanceRadius = 70.0f);
	void StopMove(EIsometricPathFollowResult Result = EIsometricPathFollowResult::Aborted);

	bool HasActiveMove() const { return bHasActiveMove; }
	bool IsFollowingActor() const { return TargetActor.IsValid(); }

	FIsometricPathFollowResultDelegate OnMoveFinished;

private:
	bool ResolveGoalLocation(FVector& OutGoalLocation) const;
	bool BuildPathToGoal(const FVector& InGoalLocation);
	bool ShouldDriveMovement() const;
	bool CanDriveMovement() const;
	void FinishMove(EIsometricPathFollowResult Result);
	void AdvancePathCursor(const FVector& ActorLocation, const FVector& GoalLocation);
	int32 GetInitialPathIndex(const FVector& ActorLocation) const;
	void MaybeRepath(float WorldTimeSeconds);

private:
	TWeakObjectPtr<ACharacter> OwnerCharacter;
	TWeakObjectPtr<AActor> TargetActor;

	TArray<FVector> PathPoints;
	FVector TargetLocation = FVector::ZeroVector;
	FVector LastResolvedGoalLocation = FVector::ZeroVector;

	float AcceptanceRadius = 35.0f;
	float WaypointAcceptanceRadius = 30.0f;
	float ActorRepathInterval = 0.15f;
	float ActorRepathDistance = 50.0f;
	float NextRepathTimeSeconds = 0.0f;

	int32 CurrentPathIndex = INDEX_NONE;
	bool bHasActiveMove = false;
	bool bHasStaticTarget = false;
	bool bPathIsPartial = false;
};
