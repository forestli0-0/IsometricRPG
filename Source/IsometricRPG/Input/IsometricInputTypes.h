#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "IsometricAbilities/Types/HeroAbilityTypes.h"
#include "IsometricInputTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EInputEventPhase : uint8
{
    Pressed,
    Held,
    Released,
    Cancelled
};

UENUM(BlueprintType)
enum class EPlayerInputSourceKind : uint8
{
    LeftMouse,
    RightMouse,
    AbilitySlot
};

UENUM(BlueprintType)
enum class EAbilityInputMode : uint8
{
    Instant,
    PressAndRelease,
    ChargeOnHold,
    ChannelOnHold,
    Toggle
};

UENUM(BlueprintType)
enum class EPlayerInputCommandType : uint8
{
    None,
    SelectTarget,
    ClearSelection,
    MoveToLocation,
    BasicAttackTarget,
    BeginAbilityInput,
    UpdateAbilityInput,
    CommitAbilityInput,
    CancelAbilityInput
};

USTRUCT(BlueprintType)
struct FCursorInputSnapshot
{
    GENERATED_BODY()

    UPROPERTY()
    EPlayerInputSourceKind SourceKind = EPlayerInputSourceKind::LeftMouse;

    UPROPERTY()
    EInputEventPhase Phase = EInputEventPhase::Pressed;

    UPROPERTY()
    EAbilityInputID AbilityInputID = EAbilityInputID::None;

    UPROPERTY()
    FHitResult HitResult;

    UPROPERTY()
    TObjectPtr<AActor> HitActor = nullptr;

    UPROPERTY()
    bool bBlockingHit = false;

    UPROPERTY()
    bool bHitEnemy = false;

    UPROPERTY()
    float WorldTime = 0.0f;

    UPROPERTY()
    float HeldDuration = 0.0f;
};

USTRUCT(BlueprintType)
struct FAbilityInputPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    EAbilityInputMode InputMode = EAbilityInputMode::Instant;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bUpdateTargetWhileHeld = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bAllowInputBuffer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0"))
    float MaxBufferWindow = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0"))
    float MinChargeSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0"))
    float MaxChargeSeconds = 1.5f;
};

USTRUCT(BlueprintType)
struct FPlayerInputCommand
{
    GENERATED_BODY()

    UPROPERTY()
    EPlayerInputCommandType Type = EPlayerInputCommandType::None;

    UPROPERTY()
    EAbilityInputID AbilityInputID = EAbilityInputID::None;

    UPROPERTY()
    EInputEventPhase TriggerPhase = EInputEventPhase::Pressed;

    UPROPERTY()
    FHitResult HitResult;

    UPROPERTY()
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY()
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY()
    float WorldTime = 0.0f;

    UPROPERTY()
    float HeldDuration = 0.0f;

    UPROPERTY()
    bool bAllowBuffering = false;
};

USTRUCT()
struct FAbilityInputSession
{
    GENERATED_BODY()

    UPROPERTY()
    EAbilityInputID AbilityInputID = EAbilityInputID::None;

    UPROPERTY()
    EAbilityInputMode InputMode = EAbilityInputMode::Instant;

    UPROPERTY()
    float StartWorldTime = 0.0f;

    UPROPERTY()
    float LastUpdateWorldTime = 0.0f;

    UPROPERTY()
    float HeldDuration = 0.0f;

    UPROPERTY()
    FHitResult LatestHitResult;

    UPROPERTY()
    TObjectPtr<AActor> LatestTargetActor = nullptr;

    UPROPERTY()
    bool bIsActive = false;
};

USTRUCT()
struct FBufferedPlayerInputCommand
{
    GENERATED_BODY()

    UPROPERTY()
    FPlayerInputCommand Command;

    UPROPERTY()
    float ExpireWorldTime = 0.0f;

    UPROPERTY()
    bool bOccupied = false;
};

USTRUCT(BlueprintType)
struct FPendingAbilityActivationContext
{
    GENERATED_BODY()

    UPROPERTY()
    EAbilityInputID InputID = EAbilityInputID::None;

    UPROPERTY()
    EInputEventPhase TriggerPhase = EInputEventPhase::Pressed;

    UPROPERTY()
    bool bUseActorTarget = false;

    UPROPERTY()
    bool bIsHeldInput = false;

    UPROPERTY()
    float HeldDuration = 0.0f;

    UPROPERTY()
    FHitResult HitResult;

    UPROPERTY()
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY()
    FVector AimPoint = FVector::ZeroVector;

    UPROPERTY()
    FVector AimDirection = FVector::ZeroVector;

    FGameplayAbilityTargetDataHandle TargetData;
};
