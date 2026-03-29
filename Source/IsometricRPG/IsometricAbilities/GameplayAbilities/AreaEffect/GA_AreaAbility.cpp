
#include "GA_AreaAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"

UGA_AreaAbility::UGA_AreaAbility()
{
	AbilityType = EHeroAbilityType::AreaEffect;
	SetUsesInteractiveTargeting(false);
	AreaRadius = 300.f;
}



void UGA_AreaAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
   AActor* SelfActor = ActorInfo->AvatarActor.Get();
   const FVector AoECenterLocation = GetCurrentAimPoint();
    if (!SelfActor || GetCurrentAimDirection().IsNearlyZero())
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot execute area skill - AimPoint is invalid."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

   UE_LOG(LogTemp, Log, TEXT("%s: Executing area skill at location %s with radius %.2f."), *GetName(), *AoECenterLocation.ToString(), AreaRadius);

   // 基类当前只负责解析 AoE 中心点，效果应用逻辑应由具体技能覆写。
   // Example: Find overlapping actors and apply an effect
   TArray<AActor*> OverlappingActors;
   TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
   ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // Example: affect only Pawns

   // UKismetSystemLibrary::SphereOverlapActors(GetWorld(), AoECenterLocation, AreaRadius, ObjectTypes, nullptr, TArray<AActor*>(), OverlappingActors);
   // For C++ only, you might use GetWorld()->OverlapMultiByObjectType or similar.
   // This often requires more setup (collision channels, etc.)

   // A simpler way for GameplayAbilities is to use a GameplayEffect with an AreaOfEffect shape.
   // Or, spawn an actor (e.g., a decal or a temporary volume) that applies effects.

   // For now, just logging. Replace with actual AoE logic.
   // for (AActor* OverlappedActor : OverlappingActors)
   // {
   //     if (OverlappedActor != SelfActor) // Don't affect self unless intended
   //     {
   //         UE_LOG(LogTemp, Log, TEXT("%s: Actor %s is in AoE."), *GetName(), *OverlappedActor->GetName());
   //         // Apply GameplayEffect to OverlappedActor
   //     }
   // }

   // If this ability is instant (no montage) and should end immediately after execution.
   if (!MontageToPlay)
   {
       EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
   }
}
