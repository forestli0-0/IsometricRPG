
#include "GA_AreaAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"

UGA_AreaAbility::UGA_AreaAbility()
{
	// Constructor logic if needed
	AreaRadius = 300.f;
}



void UGA_AreaAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
   Super::ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);

   AActor* SelfActor = ActorInfo->AvatarActor.Get();
   if (!SelfActor || !TriggerEventData || TriggerEventData->TargetData.Num() == 0 || !TriggerEventData->TargetData.Data[0].IsValid())
   {
       UE_LOG(LogTemp, Error, TEXT("%s: Cannot execute area skill - invalid prerequisites."), *GetName());
       EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // Cancel
       return;
   }

   const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Data[0].Get();
   FVector AoECenterLocation = TargetData->GetHitResult() ? FVector(TargetData->GetHitResult()->Location) : FVector(TargetData->GetEndPoint());

   UE_LOG(LogTemp, Log, TEXT("%s: Executing area skill at location %s with radius %.2f."), *GetName(), *AoECenterLocation.ToString(), AreaRadius);

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
