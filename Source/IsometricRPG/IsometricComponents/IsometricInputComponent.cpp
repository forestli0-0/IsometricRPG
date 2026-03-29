#include "IsometricInputComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "GameFramework/PlayerController.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "IsometricComponents/IsometricPathFollowingComponent.h"

UIsometricInputComponent::UIsometricInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UIsometricInputComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AIsometricRPGCharacter>(GetOwner());
}

void UIsometricInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PumpBufferedAbilityCommand();
}

void UIsometricInputComponent::ProcessInputSnapshot(const FCursorInputSnapshot& Snapshot)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (!OwnerASC)
	{
		OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
	}

	if (Snapshot.SourceKind == EPlayerInputSourceKind::LeftMouse
		&& Snapshot.Phase == EInputEventPhase::Pressed
		&& OwnerASC)
	{
		SendConfirmTargetInput();
	}
	else if (Snapshot.SourceKind == EPlayerInputSourceKind::RightMouse
		&& (Snapshot.Phase == EInputEventPhase::Pressed || Snapshot.Phase == EInputEventPhase::Held)
		&& OwnerASC)
	{
		SendCancelTargetInput();
	}

	FPlayerInputCommand Command;
	if (Snapshot.SourceKind == EPlayerInputSourceKind::AbilitySlot)
	{
		FAbilityInputPolicy Policy;
		ResolveAbilityInputPolicy(Snapshot.AbilityInputID, Policy);

		if (Snapshot.Phase == EInputEventPhase::Released && Policy.InputMode == EAbilityInputMode::Instant)
		{
			if (OwnerASC)
			{
				OwnerASC->AbilityLocalInputReleased((int32)Snapshot.AbilityInputID);
			}
			return;
		}

		Command = InputRouter.RouteAbilityInput(Snapshot, Policy);
	}
	else
	{
		Command = InputRouter.RoutePointerInput(Snapshot);
	}

	if (Command.Type == EPlayerInputCommandType::None)
	{
		return;
	}

	ExecuteCommand(Command);
}

void UIsometricInputComponent::ExecuteCommand(const FPlayerInputCommand& Command)
{
	switch (Command.Type)
	{
	case EPlayerInputCommandType::SelectTarget:
	case EPlayerInputCommandType::ClearSelection:
		ApplySelectionCommand(Command);
		break;

	case EPlayerInputCommandType::MoveToLocation:
		RequestMoveToLocation(Command.TargetLocation, Command.TriggerPhase == EInputEventPhase::Held);
		break;

	case EPlayerInputCommandType::BasicAttackTarget:
		UpdatePendingAbilityContext(Command);
		RequestBasicAttack(Command.HitResult, Command.TargetActor.Get(), Command.TriggerPhase == EInputEventPhase::Held);
		break;

	case EPlayerInputCommandType::BeginAbilityInput:
	case EPlayerInputCommandType::UpdateAbilityInput:
	case EPlayerInputCommandType::CommitAbilityInput:
	case EPlayerInputCommandType::CancelAbilityInput:
		ExecuteAbilityInputCommand(Command);
		break;

	default:
		break;
	}
}

void UIsometricInputComponent::ApplySelectionCommand(const FPlayerInputCommand& Command)
{
	if (Command.Type == EPlayerInputCommandType::SelectTarget && Command.TargetActor && Command.TargetActor != OwnerCharacter)
	{
		CurrentSelectedTargetForUI = Command.TargetActor;
		OnTargetSelectedForUI(Command.TargetActor.Get());
		return;
	}

	CurrentSelectedTargetForUI = nullptr;
	OnTargetClearedForUI();
}

void UIsometricInputComponent::ExecuteAbilityInputCommand(const FPlayerInputCommand& Command)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (!OwnerASC)
	{
		OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
	}

	if (!OwnerASC)
	{
		return;
	}

	FAbilityInputPolicy Policy;
	ResolveAbilityInputPolicy(Command.AbilityInputID, Policy);

	switch (Command.Type)
	{
	case EPlayerInputCommandType::BeginAbilityInput:
		InputSessionManager.BeginSession(Command, Policy);
		UpdatePendingAbilityContext(Command);
		break;

	case EPlayerInputCommandType::UpdateAbilityInput:
	{
		FAbilityInputSession Session;
		if (!InputSessionManager.UpdateSession(Command, Policy, Session))
		{
			return;
		}

		FPlayerInputCommand UpdatedCommand = Command;
		UpdatedCommand.HeldDuration = Session.HeldDuration;
		UpdatedCommand.TargetActor = Session.LatestTargetActor;
		UpdatedCommand.HitResult = Session.LatestHitResult;
		UpdatePendingAbilityContext(UpdatedCommand);
		break;
	}

	case EPlayerInputCommandType::CommitAbilityInput:
		if (Command.bAllowBuffering && InputSessionManager.HasBlockingSession(Command.AbilityInputID))
		{
			InputSessionManager.BufferCommand(Command, Policy);
			SetComponentTickEnabled(true);
			return;
		}

		ExecuteAbilityCommitCommand(Command, Policy);
		break;

	case EPlayerInputCommandType::CancelAbilityInput:
	{
		FAbilityInputSession Session;
		if (InputSessionManager.CancelSession(Command.AbilityInputID, Session))
		{
			FPlayerInputCommand CancelCommand = Command;
			CancelCommand.HeldDuration = Session.HeldDuration;
			CancelCommand.TargetActor = Session.LatestTargetActor;
			CancelCommand.HitResult = Session.LatestHitResult;
			UpdatePendingAbilityContext(CancelCommand);
		}

		OwnerCharacter->ClearPendingAbilityActivationContext();
		OwnerASC->AbilityLocalInputReleased((int32)Command.AbilityInputID);
		break;
	}

	default:
		break;
	}
}

bool UIsometricInputComponent::ExecuteAbilityCommitCommand(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy)
{
	if (!OwnerASC || Command.AbilityInputID == EAbilityInputID::None)
	{
		return false;
	}

	FPlayerInputCommand FinalCommand = Command;
	FAbilityInputSession Session;
	if (InputSessionManager.CommitSession(Command, Policy, Session))
	{
		FinalCommand.HeldDuration = Session.HeldDuration;
		FinalCommand.TargetActor = Session.LatestTargetActor;
		FinalCommand.HitResult = Session.LatestHitResult;
	}

	if (Policy.MaxChargeSeconds > 0.0f)
	{
		FinalCommand.HeldDuration = FMath::Min(FinalCommand.HeldDuration, Policy.MaxChargeSeconds);
	}

	UpdatePendingAbilityContext(FinalCommand);
	OwnerASC->AbilityLocalInputPressed((int32)FinalCommand.AbilityInputID);

	if (Policy.InputMode != EAbilityInputMode::Instant)
	{
		OwnerASC->AbilityLocalInputReleased((int32)FinalCommand.AbilityInputID);
	}

	return true;
}

bool UIsometricInputComponent::ResolveAbilityInputPolicy(EAbilityInputID InputID, FAbilityInputPolicy& OutPolicy) const
{
	OutPolicy = FAbilityInputPolicy();

	if (!OwnerCharacter)
	{
		return false;
	}

	const ESkillSlot Slot = ResolveSkillSlotFromInput(InputID);
	if (Slot == ESkillSlot::Invalid || Slot == ESkillSlot::MAX)
	{
		return false;
	}

	const FEquippedAbilityInfo AbilityInfo = OwnerCharacter->GetEquippedAbilityInfo(Slot);
	if (AbilityInfo.AbilityClass.ToSoftObjectPath().IsNull())
	{
		return false;
	}

	TSubclassOf<UGameplayAbility> LoadedAbilityClass = AbilityInfo.AbilityClass.LoadSynchronous();
	if (!LoadedAbilityClass)
	{
		return false;
	}

	const UGA_HeroBaseAbility* HeroAbility = Cast<UGA_HeroBaseAbility>(LoadedAbilityClass.GetDefaultObject());
	if (!HeroAbility)
	{
		return false;
	}

	OutPolicy = HeroAbility->GetAbilityInputPolicy();
	return true;
}

ESkillSlot UIsometricInputComponent::ResolveSkillSlotFromInput(EAbilityInputID InputID) const
{
	switch (InputID)
	{
	case EAbilityInputID::Ability_Q: return ESkillSlot::Skill_Q;
	case EAbilityInputID::Ability_W: return ESkillSlot::Skill_W;
	case EAbilityInputID::Ability_E: return ESkillSlot::Skill_E;
	case EAbilityInputID::Ability_R: return ESkillSlot::Skill_R;
	case EAbilityInputID::Ability_Summoner1: return ESkillSlot::Skill_D;
	case EAbilityInputID::Ability_Summoner2: return ESkillSlot::Skill_F;
	default: return ESkillSlot::Invalid;
	}
}

void UIsometricInputComponent::UpdatePendingAbilityContext(const FPlayerInputCommand& Command)
{
	if (!OwnerCharacter)
	{
		return;
	}

	const FPendingAbilityActivationContext Context = BuildActivationContext(Command);
	OwnerCharacter->SetPendingAbilityActivationContext(Context);
	SyncActivationContextToServer(Context);
}

void UIsometricInputComponent::RequestMoveToLocation(const FVector& TargetLocation, bool bIsHeldInput)
{
	UIsometricPathFollowingComponent* PathFollower = OwnerCharacter ? OwnerCharacter->PathFollowingComponent : nullptr;

	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestMoveToLocation failed: OwnerCharacter is null."));
		return;
	}

	if (PathFollower && OwnerCharacter->IsLocallyControlled())
	{
		PathFollower->RequestMoveToLocation(TargetLocation);
	}

	if (!OwnerCharacter->HasAuthority())
	{
		if (!bIsHeldInput)
		{
			Server_RequestMoveToLocation(TargetLocation);
		}
		return;
	}

	HandleMoveIntentOnAuthority();
}

void UIsometricInputComponent::RequestBasicAttack(const FHitResult& HitResult, AActor* TargetActor, bool bUseUnreliableRemoteUpdate)
{
	AController* OwnerController = OwnerCharacter ? OwnerCharacter->GetController() : nullptr;
	UAbilitySystemComponent* CurrentASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;

	if (!OwnerCharacter || !CurrentASC || !TargetActor || !OwnerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestBasicAttack failed: OwnerCharacter (%p), Controller (%p), ASC (%p), or TargetActor (%p) is null."), OwnerCharacter, OwnerController, CurrentASC, TargetActor);
		return;
	}

	StopPredictedClickMove();

	if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
	{
		if (bUseUnreliableRemoteUpdate)
		{
			Server_UpdateBasicAttack(HitResult, TargetActor);
		}
		else
		{
			Server_RequestBasicAttack(HitResult, TargetActor);
		}
		return;
	}

	const bool bHasValidHitTarget = (HitResult.bBlockingHit || HitResult.GetActor() != nullptr)
		&& (!TargetActor || HitResult.GetActor() == TargetActor);

	if (bHasValidHitTarget)
	{
		OwnerCharacter->SetAbilityTargetDataByHit(HitResult);
	}
	else if (!OwnerCharacter->HasStoredTargetActor(TargetActor))
	{
		OwnerCharacter->SetAbilityTargetDataByActor(TargetActor);
	}

	FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.DirBasicAttack"));
	if (BasicAttackAbilityTag.IsValid())
	{
		if (CurrentASC->TryActivateAbilitiesByTag(FGameplayTagContainer(BasicAttackAbilityTag), true))
		{
			UE_LOG(LogTemp, Display, TEXT("RequestBasicAttack: Successfully activated Basic Attack ability with tag %s."), *BasicAttackAbilityTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("RequestBasicAttack: Failed to activate Basic Attack ability with tag %s."), *BasicAttackAbilityTag.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Basic Attack ability tag is invalid for RequestBasicAttack."));
	}
}

void UIsometricInputComponent::SendConfirmTargetInput()
{
	if (OwnerASC)
	{
		OwnerASC->AbilityLocalInputPressed((int32)EAbilityInputID::Confirm);
	}
}

void UIsometricInputComponent::SendCancelTargetInput()
{
	if (OwnerASC)
	{
		OwnerASC->AbilityLocalInputPressed((int32)EAbilityInputID::Cancel);
	}
}

void UIsometricInputComponent::HandleMoveIntentOnAuthority()
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (UAbilitySystemComponent* CurrentASC = OwnerCharacter->GetAbilitySystemComponent())
	{
		FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.BasicAttack"));
		FGameplayTag DirBasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.DirBasicAttack"));
		if (BasicAttackAbilityTag.IsValid())
		{
			FGameplayTagContainer TagContainer(BasicAttackAbilityTag);
			TagContainer.AddTag(DirBasicAttackAbilityTag);
			CurrentASC->CancelAbilities(&TagContainer);
			UE_LOG(LogTemp, Log, TEXT("Server: Canceled attack abilities."));
		}
	}

	if (OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Stop(0.1f);
		UE_LOG(LogTemp, Log, TEXT("Server: Stopped any active montage."));
	}
}

void UIsometricInputComponent::StopPredictedClickMove()
{
	if (!(OwnerCharacter && OwnerCharacter->IsLocallyControlled()))
	{
		return;
	}

	if (OwnerCharacter->PathFollowingComponent)
	{
		OwnerCharacter->PathFollowingComponent->StopMove();
	}
}

void UIsometricInputComponent::Server_RequestMoveToLocation_Implementation(const FVector& TargetLocation)
{
	HandleMoveIntentOnAuthority();
}

void UIsometricInputComponent::Server_RequestBasicAttack_Implementation(const FHitResult& HitResult, AActor* TargetActor)
{
	RequestBasicAttack(HitResult, TargetActor);
}

void UIsometricInputComponent::Server_UpdateBasicAttack_Implementation(const FHitResult& HitResult, AActor* TargetActor)
{
	RequestBasicAttack(HitResult, TargetActor);
}

FPendingAbilityActivationContext UIsometricInputComponent::BuildActivationContext(const FPlayerInputCommand& Command) const
{
	FPendingAbilityActivationContext Context;
	Context.InputID = Command.AbilityInputID;
	Context.TriggerPhase = Command.TriggerPhase;
	Context.bIsHeldInput = Command.TriggerPhase == EInputEventPhase::Held;
	Context.HeldDuration = Command.HeldDuration;
	Context.HitResult = Command.HitResult;
	Context.TargetActor = Command.TargetActor;
	Context.bUseActorTarget = false;

	if (Context.HitResult.bBlockingHit || Context.HitResult.GetActor() != nullptr)
	{
		Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Context.HitResult);
	}
	else if (Context.TargetActor)
	{
		Context.bUseActorTarget = true;
		Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Context.TargetActor.Get());
	}
	else
	{
		Context.TargetData.Clear();
	}

	if (Context.HitResult.bBlockingHit || Context.HitResult.GetActor() != nullptr)
	{
		Context.AimPoint = !Context.HitResult.Location.IsNearlyZero()
			? Context.HitResult.Location
			: Context.HitResult.ImpactPoint;
	}
	else if (Context.TargetActor)
	{
		Context.AimPoint = Context.TargetActor->GetActorLocation();
	}
	else
	{
		Context.AimPoint = Command.TargetLocation;
	}

	if (OwnerCharacter)
	{
		Context.AimDirection = Context.AimPoint - OwnerCharacter->GetActorLocation();
		Context.AimDirection.Z = 0.0f;
		Context.AimDirection = Context.AimDirection.GetSafeNormal();
	}

	return Context;
}

void UIsometricInputComponent::SyncActivationContextToServer(const FPendingAbilityActivationContext& Context) const
{
	if (!OwnerCharacter || OwnerCharacter->GetLocalRole() >= ROLE_Authority)
	{
		return;
	}

	if (Context.TriggerPhase == EInputEventPhase::Held)
	{
		OwnerCharacter->Server_UpdatePendingAbilityActivationContext(
			Context.InputID,
			Context.TriggerPhase,
			Context.bUseActorTarget,
			Context.bIsHeldInput,
			Context.HeldDuration,
			Context.HitResult,
			Context.TargetActor.Get());
		return;
	}

	OwnerCharacter->Server_SetPendingAbilityActivationContext(
		Context.InputID,
		Context.TriggerPhase,
		Context.bUseActorTarget,
		Context.bIsHeldInput,
		Context.HeldDuration,
		Context.HitResult,
		Context.TargetActor.Get());
}

void UIsometricInputComponent::PumpBufferedAbilityCommand()
{
	if (!InputSessionManager.HasBufferedCommand())
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (InputSessionManager.HasAnyActiveSession())
	{
		return;
	}

	FPlayerInputCommand BufferedCommand;
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (!InputSessionManager.ConsumeBufferedCommand(WorldTime, BufferedCommand))
	{
		SetComponentTickEnabled(false);
		return;
	}

	BufferedCommand.WorldTime = WorldTime;
	ExecuteCommand(BufferedCommand);
}


