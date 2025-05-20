// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_Death.cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Death.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/ABP_MyCharacterAnimInstance.h"
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
            // ... disable input, movement, collision as before ...
            Character->DisableInput(nullptr);
            Character->GetCharacterMovement()->DisableMovement();
            Character->SetActorEnableCollision(false);

            // --- 在这里获取并设置动画蓝图变量 ---
            UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
            if (AnimInstance)
            {
                // 尝试将通用的 AnimInstance 转换为你的特定动画蓝图类
                auto CharacterAnimBP = Cast<UABP_MyCharacterAnimInstance>(AnimInstance);
                if (CharacterAnimBP)
                {
                    CharacterAnimBP->SetIsDead(true); // 通过函数设置死亡标志
                    Character->SetLifeSpan(3.f);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to cast AnimInstance to UMyCharacterAnimBP. Check character's AnimBP class."));
                    // 处理转换失败的情况，可能角色没有使用正确的动画蓝图
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Character has no AnimInstance."));
            }
        }
    }
}
