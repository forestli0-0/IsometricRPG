#include "IsometricAbilities/GameplayAbilities/Targeted/HeroTargetedAbilityPresentationHelper.h"

#include "Character/IsometricRPGCharacter.h"
#include "GameFramework/Character.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetDataHelper.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/GA_TargetedAbility.h"

void FHeroTargetedAbilityPresentationHelper::SpawnRangeIndicator(UGA_TargetedAbility& Ability, const float RangeToApply)
{
    if (!Ability.RangeIndicatorNiagaraActorClass)
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: RangeIndicatorNiagaraActorClass is not configured. Continuing without indicator."), *Ability.GetName());
        return;
    }

    AActor* Avatar = Ability.GetAvatarActorFromActorInfo();
    ACharacter* AvatarCharacter = Cast<ACharacter>(Avatar);
    if (!AvatarCharacter || !AvatarCharacter->GetMesh())
    {
        ensureAlwaysMsgf(
            AvatarCharacter && AvatarCharacter->GetMesh(),
            TEXT("%s: Cannot attach range indicator because avatar %s is not a character with a valid mesh. Continuing without indicator."),
            *Ability.GetName(),
            *GetNameSafe(Avatar));
        return;
    }

    ANiagaraActor* NiagaraRangeIndicator = Ability.GetWorld()->SpawnActor<ANiagaraActor>(
        Ability.RangeIndicatorNiagaraActorClass,
        AvatarCharacter->GetMesh()->GetComponentLocation(),
        FRotator::ZeroRotator);

    if (!NiagaraRangeIndicator)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to spawn NiagaraActor for RangeIndicator. Continuing without indicator."), *Ability.GetName());
        return;
    }

    NiagaraRangeIndicator->AttachToComponent(
        AvatarCharacter->GetMesh(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        FName("Root"));

    if (NiagaraRangeIndicator->GetNiagaraComponent())
    {
        NiagaraRangeIndicator->GetNiagaraComponent()->SetFloatParameter(FName("user_Radius"), RangeToApply);
    }

    Ability.ActiveRangeIndicatorNiagaraActor = NiagaraRangeIndicator;
}

void FHeroTargetedAbilityPresentationHelper::DestroyRangeIndicator(TObjectPtr<ANiagaraActor>& ActiveRangeIndicatorNiagaraActor)
{
    if (ActiveRangeIndicatorNiagaraActor && IsValid(ActiveRangeIndicatorNiagaraActor))
    {
        ActiveRangeIndicatorNiagaraActor->Destroy();
        ActiveRangeIndicatorNiagaraActor = nullptr;
    }
}

void FHeroTargetedAbilityPresentationHelper::SyncCurrentAbilityTargets(AActor* AvatarActor, const FGameplayAbilityTargetDataHandle& Data)
{
    AIsometricRPGCharacter* MyCharacter = Cast<AIsometricRPGCharacter>(AvatarActor);
    if (!MyCharacter)
    {
        return;
    }

    MyCharacter->CurrentAbilityTargets.Empty();
    for (int32 TargetDataIndex = 0; TargetDataIndex < Data.Num(); ++TargetDataIndex)
    {
        const FGameplayAbilityTargetData* TargetData = Data.Get(TargetDataIndex);
        if (!TargetData)
        {
            continue;
        }

        if (AActor* Actor = FHeroAbilityTargetDataHelper::GetPrimaryActor(*TargetData))
        {
            MyCharacter->CurrentAbilityTargets.AddUnique(Actor);
        }

        for (const TWeakObjectPtr<AActor>& WeakActor : TargetData->GetActors())
        {
            if (AActor* Actor = WeakActor.Get())
            {
                MyCharacter->CurrentAbilityTargets.AddUnique(Actor);
            }
        }
    }
}

void FHeroTargetedAbilityPresentationHelper::ClearCurrentAbilityTargets(AActor* AvatarActor)
{
    if (AIsometricRPGCharacter* MyCharacter = Cast<AIsometricRPGCharacter>(AvatarActor))
    {
        MyCharacter->CurrentAbilityTargets.Empty();
    }
}
