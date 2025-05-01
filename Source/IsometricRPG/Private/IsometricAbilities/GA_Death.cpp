// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/GA_Death.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
UGA_Death::UGA_Death()
{
    // 设定为立即生效的技能
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 初始化死亡蒙太奇路径
    static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageObj(TEXT("/Game/Characters/Gideon/Animations/Death_Fwd_Montage"));
    if (AttackMontageObj.Succeeded())
    {
        DeathMontage = AttackMontageObj.Object;
    }
}


// Add the missing OnMontageEnded method to the UGA_Death class  
void UGA_Death::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)  
{  
   AActor* AvatarActor = GetAvatarActorFromActorInfo();  
   if (AvatarActor)  
   {  
       ACharacter* Character = Cast<ACharacter>(AvatarActor);  
       if (Character)  
       {  
           // Destroy the character after the montage ends  
           Character->Destroy();  
       }  
   }  
}  

void UGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)  
{  
   if (!CommitAbility(Handle, ActorInfo, ActivationInfo))  
   {  
       EndAbility(Handle, ActorInfo, ActivationInfo, true, false);  
       return;  
   }  
   Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
   AActor* AvatarActor = GetAvatarActorFromActorInfo();  
   if (AvatarActor)  
   {  
       ACharacter* Character = Cast<ACharacter>(AvatarActor);  
       if (Character)  
       {  
           Character->DisableInput(nullptr);  
           Character->GetCharacterMovement()->DisableMovement();
           Character->SetActorEnableCollision(false);  

           UAnimMontage* MontageToPlay = DeathMontage;  
           if (MontageToPlay)  
           {  
               UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();  
               if (AnimInstance)  
               {  
                   float MontagePlayResult = AnimInstance->Montage_Play(MontageToPlay);
                   if (MontagePlayResult == 0.0f)
                   {
                       UE_LOG(LogTemp, Error, TEXT("Failed to play DeathMontage. Check if the montage is valid and compatible with the animation blueprint."));
                       return;
                   }
                   FOnMontageEnded MontageEndDelegate = FOnMontageEnded::CreateUObject(this, &UGA_Death::OnMontageEnded);  
                   AnimInstance->Montage_SetEndDelegate(MontageEndDelegate);  
               }  
           }  

           if (!DeathMontage)  
           {  
               Character->Destroy();  
           }  
       }  
       else  
       {  
           UE_LOG(LogTemp, Warning, TEXT("AvatarActor is not a character"));  
       }  
   }  
}
