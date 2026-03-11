#include "IsometricPathFollowingComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

UIsometricPathFollowingComponent::UIsometricPathFollowingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UIsometricPathFollowingComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
}

void UIsometricPathFollowingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bHasActiveMove)
	{
		FinishMove(EIsometricPathFollowResult::Aborted);
	}

	Super::EndPlay(EndPlayReason);
}

void UIsometricPathFollowingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasActiveMove)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (!OwnerCharacter.IsValid())
	{
		return;
	}

	if (!ShouldDriveMovement())
	{
		return;
	}

	if (!CanDriveMovement())
	{
		FinishMove(EIsometricPathFollowResult::Aborted);
		return;
	}

	FVector GoalLocation = FVector::ZeroVector;
	if (!ResolveGoalLocation(GoalLocation))
	{
		FinishMove(EIsometricPathFollowResult::Failed);
		return;
	}

	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const FVector DeltaToGoal = GoalLocation - ActorLocation;
	const float GoalDistanceSq2D = FVector2D(DeltaToGoal.X, DeltaToGoal.Y).SizeSquared();
	if (GoalDistanceSq2D <= FMath::Square(AcceptanceRadius))
	{
		FinishMove(EIsometricPathFollowResult::Succeeded);
		return;
	}

	if (TargetActor.IsValid())
	{
		MaybeRepath(GetWorld()->GetTimeSeconds());
	}

	if (PathPoints.Num() == 0)
	{
		FinishMove(EIsometricPathFollowResult::Failed);
		return;
	}

	AdvancePathCursor(ActorLocation, GoalLocation);
	if (!PathPoints.IsValidIndex(CurrentPathIndex))
	{
		FinishMove(bPathIsPartial ? EIsometricPathFollowResult::Failed : EIsometricPathFollowResult::Succeeded);
		return;
	}

	FVector MoveDirection = PathPoints[CurrentPathIndex] - ActorLocation;
	MoveDirection.Z = 0.0f;
	if (MoveDirection.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	OwnerCharacter->AddMovementInput(MoveDirection.GetSafeNormal());
}

bool UIsometricPathFollowingComponent::RequestMoveToLocation(const FVector& InTargetLocation, float InAcceptanceRadius)
{
	if (!OwnerCharacter.IsValid())
	{
		return false;
	}

	if (bHasActiveMove)
	{
		FinishMove(EIsometricPathFollowResult::Aborted);
	}

	TargetActor.Reset();
	TargetLocation = InTargetLocation;
	AcceptanceRadius = FMath::Max(InAcceptanceRadius, 10.0f);
	bHasStaticTarget = true;

	if (!ShouldDriveMovement())
	{
		return false;
	}

	if (!BuildPathToGoal(TargetLocation))
	{
		FinishMove(EIsometricPathFollowResult::Failed);
		return false;
	}

	bHasActiveMove = true;
	SetComponentTickEnabled(true);
	return true;
}

bool UIsometricPathFollowingComponent::RequestMoveToActor(AActor* InTargetActor, float InAcceptanceRadius)
{
	if (!(OwnerCharacter.IsValid() && InTargetActor))
	{
		return false;
	}

	if (bHasActiveMove)
	{
		FinishMove(EIsometricPathFollowResult::Aborted);
	}

	TargetActor = InTargetActor;
	AcceptanceRadius = FMath::Max(InAcceptanceRadius, 10.0f);
	bHasStaticTarget = false;

	if (!ShouldDriveMovement())
	{
		return false;
	}

	FVector GoalLocation = FVector::ZeroVector;
	if (!ResolveGoalLocation(GoalLocation) || !BuildPathToGoal(GoalLocation))
	{
		FinishMove(EIsometricPathFollowResult::Failed);
		return false;
	}

	bHasActiveMove = true;
	SetComponentTickEnabled(true);
	return true;
}

void UIsometricPathFollowingComponent::StopMove(EIsometricPathFollowResult Result)
{
	if (!bHasActiveMove && Result == EIsometricPathFollowResult::Aborted)
	{
		return;
	}

	FinishMove(Result);
}

bool UIsometricPathFollowingComponent::ResolveGoalLocation(FVector& OutGoalLocation) const
{
	if (TargetActor.IsValid())
	{
		OutGoalLocation = TargetActor->GetActorLocation();
		return true;
	}

	if (!bHasStaticTarget)
	{
		return false;
	}

	OutGoalLocation = TargetLocation;
	return true;
}

bool UIsometricPathFollowingComponent::BuildPathToGoal(const FVector& InGoalLocation)
{
	if (!(OwnerCharacter.IsValid() && GetWorld()))
	{
		return false;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSystem)
	{
		return false;
	}

	UNavigationPath* NavigationPath = NavSystem->FindPathToLocationSynchronously(
		GetWorld(),
		OwnerCharacter->GetActorLocation(),
		InGoalLocation,
		OwnerCharacter.Get());

	if (!(NavigationPath && NavigationPath->IsValid() && NavigationPath->PathPoints.Num() > 0))
	{
		PathPoints.Reset();
		CurrentPathIndex = INDEX_NONE;
		bPathIsPartial = false;
		return false;
	}

	PathPoints = NavigationPath->PathPoints;
	LastResolvedGoalLocation = InGoalLocation;
	bPathIsPartial = NavigationPath->IsPartial();
	CurrentPathIndex = GetInitialPathIndex(OwnerCharacter->GetActorLocation());
	NextRepathTimeSeconds = 0.0f;
	return PathPoints.IsValidIndex(CurrentPathIndex);
}

bool UIsometricPathFollowingComponent::ShouldDriveMovement() const
{
	if (!(OwnerCharacter.IsValid() && OwnerCharacter->GetController()))
	{
		return false;
	}

	if (OwnerCharacter->IsPlayerControlled())
	{
		return OwnerCharacter->IsLocallyControlled();
	}

	return OwnerCharacter->HasAuthority();
}

bool UIsometricPathFollowingComponent::CanDriveMovement() const
{
	if (!OwnerCharacter.IsValid())
	{
		return false;
	}

	const UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
	return CharacterMovement && CharacterMovement->MovementMode != MOVE_None;
}

void UIsometricPathFollowingComponent::FinishMove(EIsometricPathFollowResult Result)
{
	const bool bWasActive = bHasActiveMove;

	bHasActiveMove = false;
	PathPoints.Reset();
	CurrentPathIndex = INDEX_NONE;
	TargetActor.Reset();
	TargetLocation = FVector::ZeroVector;
	LastResolvedGoalLocation = FVector::ZeroVector;
	bPathIsPartial = false;
	bHasStaticTarget = false;
	NextRepathTimeSeconds = 0.0f;
	SetComponentTickEnabled(false);

	if (bWasActive)
	{
		OnMoveFinished.Broadcast(Result);
	}
}

void UIsometricPathFollowingComponent::AdvancePathCursor(const FVector& ActorLocation, const FVector& GoalLocation)
{
	while (PathPoints.IsValidIndex(CurrentPathIndex))
	{
		const bool bIsFinalPoint = (CurrentPathIndex == PathPoints.Num() - 1);
		const float CurrentAcceptance = bIsFinalPoint ? AcceptanceRadius : WaypointAcceptanceRadius;
		const FVector PathDelta = PathPoints[CurrentPathIndex] - ActorLocation;
		if (FVector2D(PathDelta.X, PathDelta.Y).SizeSquared() > FMath::Square(CurrentAcceptance))
		{
			break;
		}

		++CurrentPathIndex;
	}

	if (!PathPoints.IsValidIndex(CurrentPathIndex))
	{
		const FVector GoalDelta = GoalLocation - ActorLocation;
		if (FVector2D(GoalDelta.X, GoalDelta.Y).SizeSquared() <= FMath::Square(AcceptanceRadius))
		{
			bPathIsPartial = false;
		}
	}
}

int32 UIsometricPathFollowingComponent::GetInitialPathIndex(const FVector& ActorLocation) const
{
	if (PathPoints.Num() == 0)
	{
		return INDEX_NONE;
	}

	if (PathPoints.Num() == 1)
	{
		return 0;
	}

	const FVector FirstPointDelta = PathPoints[0] - ActorLocation;
	return FVector2D(FirstPointDelta.X, FirstPointDelta.Y).SizeSquared() <= FMath::Square(WaypointAcceptanceRadius) ? 1 : 0;
}

void UIsometricPathFollowingComponent::MaybeRepath(float WorldTimeSeconds)
{
	if (!TargetActor.IsValid())
	{
		return;
	}

	if (WorldTimeSeconds < NextRepathTimeSeconds)
	{
		return;
	}

	FVector GoalLocation = FVector::ZeroVector;
	if (!ResolveGoalLocation(GoalLocation))
	{
		return;
	}

	const FVector GoalDrift = GoalLocation - LastResolvedGoalLocation;
	if (FVector2D(GoalDrift.X, GoalDrift.Y).SizeSquared() < FMath::Square(ActorRepathDistance))
	{
		NextRepathTimeSeconds = WorldTimeSeconds + ActorRepathInterval;
		return;
	}

	if (BuildPathToGoal(GoalLocation))
	{
		bHasActiveMove = true;
	}

	NextRepathTimeSeconds = WorldTimeSeconds + ActorRepathInterval;
}
