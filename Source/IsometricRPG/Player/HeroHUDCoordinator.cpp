#include "Player/HeroHUDCoordinator.h"

#include "Abilities/GameplayAbility.h"
#include "Character/IsoPlayerState.h"
#include "Player/IsometricPlayerController.h"
#include "UI/HUD/HUDPresentationBuilder.h"
#include "UI/HUD/HUDRootWidget.h"

void FHeroHUDCoordinator::Initialize(AIsometricPlayerController& InController)
{
    OwningController = &InController;
    EnsureHUDCreated();
}

void FHeroHUDCoordinator::HandlePlayerStateChanged(AIsoPlayerState* InPlayerState)
{
    AIsometricPlayerController* Controller = nullptr;
    if (!ResolveController(Controller) || !Controller->IsLocalController())
    {
        return;
    }

    EnsureHUDCreated();

    if (BoundHUDRefreshSource.Get() == InPlayerState && HUDRefreshRequestedHandle.IsValid())
    {
        RefreshHUD(FHeroHUDRefreshRequest{ EHeroHUDRefreshKind::Full });
        return;
    }

    UnbindHUDRefreshSource();
    if (!InPlayerState)
    {
        return;
    }

    BoundHUDRefreshSource = InPlayerState;
    HUDRefreshRequestedHandle = InPlayerState->OnHUDRefreshRequested().AddRaw(this, &FHeroHUDCoordinator::HandleHUDRefreshRequested);
    RefreshHUD(FHeroHUDRefreshRequest{ EHeroHUDRefreshKind::Full });
}

void FHeroHUDCoordinator::NotifyCooldownTriggered(const FGameplayAbilitySpecHandle& SpecHandle, const float DurationSeconds)
{
    if (!SpecHandle.IsValid() || DurationSeconds <= 0.f)
    {
        return;
    }

    FHeroHUDRefreshRequest Request;
    Request.Kind = EHeroHUDRefreshKind::Cooldown;
    Request.SpecHandle = SpecHandle;
    Request.DurationSeconds = DurationSeconds;
    RefreshHUD(Request);
}

void FHeroHUDCoordinator::Reset()
{
    UnbindHUDRefreshSource();
    OwningController.Reset();
}

void FHeroHUDCoordinator::EnsureHUDCreated()
{
    AIsometricPlayerController* Controller = nullptr;
    if (!ResolveController(Controller) || !Controller->IsLocalController() || Controller->PlayerHUDInstance || !Controller->PlayerHUDClass)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[PC] Creating PlayerHUD on %s"), *Controller->GetName());
    Controller->PlayerHUDInstance = CreateWidget<UHUDRootWidget>(Controller, Controller->PlayerHUDClass);
    if (Controller->PlayerHUDInstance)
    {
        Controller->PlayerHUDInstance->AddToViewport();
    }
}

void FHeroHUDCoordinator::UnbindHUDRefreshSource()
{
    if (AIsoPlayerState* CurrentSource = BoundHUDRefreshSource.Get())
    {
        if (HUDRefreshRequestedHandle.IsValid())
        {
            CurrentSource->OnHUDRefreshRequested().Remove(HUDRefreshRequestedHandle);
        }
    }

    HUDRefreshRequestedHandle.Reset();
    BoundHUDRefreshSource.Reset();
}

void FHeroHUDCoordinator::HandleHUDRefreshRequested(const FHeroHUDRefreshRequest& Request)
{
    RefreshHUD(Request);
}

void FHeroHUDCoordinator::RefreshHUD(const FHeroHUDRefreshRequest& Request)
{
    AIsometricPlayerController* Controller = nullptr;
    if (!ResolveController(Controller) || !Controller->IsLocalController())
    {
        return;
    }

    EnsureHUDCreated();
    if (!Controller->PlayerHUDInstance)
    {
        return;
    }

    AIsoPlayerState* IsoPlayerState = ResolvePlayerState(*Controller);
    if (!IsoPlayerState)
    {
        return;
    }

    const FHUDPresentationContext Context = IsoPlayerState->MakeHUDPresentationContext();

    switch (Request.Kind)
    {
    case EHeroHUDRefreshKind::Full:
        FHUDPresentationBuilder::RefreshEntireHUD(
            *Controller->PlayerHUDInstance,
            Context,
            [IsoPlayerState](const UGameplayAbility* AbilityCDO, float& OutDuration, float& OutRemaining)
            {
                return IsoPlayerState->QueryCooldownState(AbilityCDO, OutDuration, OutRemaining);
            });
        return;

    case EHeroHUDRefreshKind::Vitals:
        FHUDPresentationBuilder::RefreshVitals(*Controller->PlayerHUDInstance, Context.AttributeSet);
        return;

    case EHeroHUDRefreshKind::Experience:
        FHUDPresentationBuilder::RefreshExperience(*Controller->PlayerHUDInstance, Context.AttributeSet);
        return;

    case EHeroHUDRefreshKind::ChampionStats:
        FHUDPresentationBuilder::RefreshChampionStats(*Controller->PlayerHUDInstance, Context.AttributeSet);
        return;

    case EHeroHUDRefreshKind::GameplayPresentation:
        FHUDPresentationBuilder::RefreshGameplayTagPresentation(*Controller->PlayerHUDInstance, Context);
        return;

    case EHeroHUDRefreshKind::ActionBar:
        if (Context.EquippedAbilities)
        {
            FHUDPresentationBuilder::RefreshActionBar(
                *Controller->PlayerHUDInstance,
                *Context.EquippedAbilities,
                [IsoPlayerState](const UGameplayAbility* AbilityCDO, float& OutDuration, float& OutRemaining)
                {
                    return IsoPlayerState->QueryCooldownState(AbilityCDO, OutDuration, OutRemaining);
                });
        }
        return;

    case EHeroHUDRefreshKind::Cooldown:
        {
            FEquippedAbilityInfo EquippedInfo;
            if (!Request.SpecHandle.IsValid() || !IsoPlayerState->TryGetEquippedAbilityInfoByHandle(Request.SpecHandle, EquippedInfo))
            {
                return;
            }

            if (EquippedInfo.Slot == ESkillSlot::Invalid || EquippedInfo.Slot == ESkillSlot::MAX)
            {
                return;
            }

            float CooldownDuration = Request.DurationSeconds;
            float CooldownRemaining = Request.DurationSeconds;

            UClass* AbilityClass = EquippedInfo.AbilityClass.Get();
            if (!AbilityClass && !EquippedInfo.AbilityClass.ToSoftObjectPath().IsNull())
            {
                AbilityClass = EquippedInfo.AbilityClass.LoadSynchronous();
            }

            if (AbilityClass)
            {
                if (const UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>())
                {
                    IsoPlayerState->QueryCooldownState(AbilityCDO, CooldownDuration, CooldownRemaining);
                }
            }

            Controller->PlayerHUDInstance->UpdateAbilityCooldown(EquippedInfo.Slot, CooldownDuration, CooldownRemaining);
        }
        return;
    }
}

bool FHeroHUDCoordinator::ResolveController(AIsometricPlayerController*& OutController) const
{
    OutController = OwningController.Get();
    return OutController != nullptr;
}

AIsoPlayerState* FHeroHUDCoordinator::ResolvePlayerState(AIsometricPlayerController& Controller) const
{
    if (AIsoPlayerState* BoundSource = BoundHUDRefreshSource.Get())
    {
        return BoundSource;
    }

    return Controller.GetPlayerState<AIsoPlayerState>();
}
