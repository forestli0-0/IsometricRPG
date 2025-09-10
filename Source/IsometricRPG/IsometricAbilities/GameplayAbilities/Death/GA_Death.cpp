// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_Death.cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Death.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/ABP_MyCharacterAnimInstance.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "ExperienceOrb.h"
#include <Character/IsometricRPGCharacter.h>
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
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag(FName("Event.Character.Death")); // 监听我们在属性集中定义的Tag
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    // 为了防止这个技能被手动激活，可以清空AbilityTags
    AbilityTags.Reset();
}


void UGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	// 死亡技能与普通技能的激活方式不一样，由于需要传入击杀者信息，所以只能通过事件触发
	// Todo: 这里一个是掉落形式应该在客户端表现，另一个是经验值的发放应该是服务器端执行
    if (ActorInfo->IsNetAuthority())
    {
        AActor* InstigatorActor = nullptr;
        if (TriggerEventData && TriggerEventData->Instigator)
        {
            InstigatorActor = const_cast<AActor*>(TriggerEventData->Instigator.Get());
        }

        // 确保击杀者存在且不是自己
        if (InstigatorActor && InstigatorActor != ActorInfo->AvatarActor.Get())
        {
			// 获取死掉的角色的属性集
            const UIsometricRPGAttributeSetBase* DeadCharacterAttributes = ActorInfo->AbilitySystemComponent->GetSet<UIsometricRPGAttributeSetBase>();
            if (DeadCharacterAttributes)
            {
                float ExpBounty = DeadCharacterAttributes->GetExperienceBounty();
                if (ExpBounty > 0)
                {
                    // --- 核心修改：生成经验球而不是直接应用GE ---
                    UWorld* World = GetWorld();
                    if (World)
                    {
                        FVector SpawnLocation = ActorInfo->AvatarActor->GetActorLocation();
                        FRotator SpawnRotation = ActorInfo->AvatarActor->GetActorRotation();

                        // 动态加载经验球蓝图类
                        FString OrbBPPath = TEXT("/Game/Blueprint/BP_ExperienceOrb.BP_ExperienceOrb_C");
                        TSubclassOf<AExperienceOrb> OrbClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), nullptr, *OrbBPPath));

                        if (OrbClass)
                        {
                            FActorSpawnParameters SpawnParams;
                            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                            AExperienceOrb* SpawnedOrb = World->SpawnActor<AExperienceOrb>(OrbClass, SpawnLocation, SpawnRotation, SpawnParams);

                            if (SpawnedOrb)
                            {
                                // 初始化经验球，告诉它飞向谁、带多少经验
                                SpawnedOrb->InitializeOrb(InstigatorActor, ExpBounty);
                                UE_LOG(LogTemp, Log, TEXT("%s dropped an experience orb for %s."), *ActorInfo->AvatarActor->GetName(), *InstigatorActor->GetName());
                            }
                        }
                    }
                }
            }
        }
    }
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
                auto MyCharacter = Cast<AIsometricRPGCharacter>(Character);
                if (MyCharacter)
                {
                    MyCharacter->SetIsDead(true); // 通过函数设置死亡标志
                    MyCharacter->SetLifeSpan(3.f);
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
