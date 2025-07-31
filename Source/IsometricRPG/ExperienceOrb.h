#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "ExperienceOrb.generated.h"

class USphereComponent;
class UAbilitySystemComponent;

UCLASS()
class ISOMETRICRPG_API AExperienceOrb : public AActor
{
    GENERATED_BODY()

public:
    AExperienceOrb();
    // 每帧调用
    virtual void Tick(float DeltaTime) override;
    /**
 * @brief 初始化经验球，设置其飞行目标和经验值
 * @param HomingTarget 经验球将飞向的目标 Actor
 * @param ExpValue 提供的经验值
 */
    UFUNCTION(BlueprintCallable, Category = "Experience Orb")
    void InitializeOrb(AActor* HomingTarget, float ExpValue);
protected:
    // 球形碰撞组件，用于检测重叠
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> SphereComponent;

    // 要应用的 GameplayEffect (在蓝图中设置为你的 GE_AddExperience)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TSubclassOf<UGameplayEffect> ExperienceGameplayEffect;

    // 这个经验球提供的经验值
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    float ExperienceAmount = 25.0f;

    // 飞向目标的速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience Orb")
    float HomingSpeed = 400.0f;
private:
    // 经验球要飞向的目标
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    // 当 Actor 开始与此组件重叠时调用的函数
    UFUNCTION()
    void OnSphereOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    virtual void BeginPlay() override;
};