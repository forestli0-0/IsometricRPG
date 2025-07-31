#include "ExperienceOrb.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGCharacter.h" // 引用你的角色类
#include "Kismet/KismetMathLibrary.h" // 添加此行以包含UKismetMathLibrary的声明

AExperienceOrb::AExperienceOrb()
{
    PrimaryActorTick.bCanEverTick = true;

    SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
    RootComponent = SphereComponent;

    // 设置碰撞配置
    SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 只与 Pawn 类型的 Actor 发生重叠事件
    SphereComponent->SetSphereRadius(100.0f);
}

void AExperienceOrb::OnSphereOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // 只对我们的飞行目标应用经验
    if (!TargetActor.IsValid() || OtherActor != TargetActor.Get())
    {
        return;
    }
    // 检查进入的 Actor 是否是你的玩家角色
    AIsometricRPGCharacter* PlayerCharacter = Cast<AIsometricRPGCharacter>(OtherActor);
    if (!PlayerCharacter)
    {
        return;
    }

    // 检查是否设置了 GE
    if (!ExperienceGameplayEffect)
    {
        UE_LOG(LogTemp, Error, TEXT("ExperienceOrb: ExperienceGameplayEffect is not set!"));
        return;
    }

    // 从角色身上获取 AbilitySystemComponent (ASC)
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter);
    if (!TargetASC)
    {
        UE_LOG(LogTemp, Error, TEXT("ExperienceOrb: Could not find AbilitySystemComponent on %s"), *PlayerCharacter->GetName());
        return;
    }

    // --- 核心逻辑：创建并应用带 SetByCaller 的 GE Spec ---

    // 1. 创建 GE Spec (规格)
    //    这里我们使用自身的ASC作为Source，但因为是场景物体，我们直接用目标的ASC来创建，并将自己设为Source Object
    FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
    ContextHandle.AddSourceObject(this); // 将经验球自身作为效果来源

    FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(ExperienceGameplayEffect, 1.0f, ContextHandle);

    if (!SpecHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("ExperienceOrb: Failed to create a valid GameplayEffectSpecHandle."));
        return;
    }

    // 2. 为 GE Spec 设置 SetByCaller 的值
    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Experience")), ExperienceAmount);

    // 3. 将修改后的 GE Spec 应用到目标 ASC
    FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

    // 检查应用是否成功
    if (AppliedHandle.WasSuccessfullyApplied())
    {
        UE_LOG(LogTemp, Warning, TEXT("Successfully applied %f experience to %s."), ExperienceAmount, *PlayerCharacter->GetName());
        // 成功应用后销毁经验球
        Destroy();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to apply experience GE to %s."), *PlayerCharacter->GetName());
    }
}


void AExperienceOrb::BeginPlay()
{
    Super::BeginPlay();
    SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AExperienceOrb::OnSphereOverlap);
}
void AExperienceOrb::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 检查目标是否存在且有效
    if (TargetActor.IsValid())
    {
        // 计算目标位置
        FVector TargetLocation = TargetActor->GetActorLocation();
        // 获取当前位置
        FVector CurrentLocation = GetActorLocation();

        // 使用 VInterpTo 平滑地移动到目标，这样看起来更自然
        FVector NewLocation = UKismetMathLibrary::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, HomingSpeed / 100.f); // InterpSpeed 需要一个较小的值

        SetActorLocation(NewLocation);
    }
    else
    {
        // 如果目标消失了（比如玩家死亡或断线），则销毁经验球，避免它永远漂浮在空中
        Destroy();
    }
}
void AExperienceOrb::InitializeOrb(AActor* HomingTarget, float ExpValue)
{
    TargetActor = HomingTarget;
    ExperienceAmount = ExpValue;
}