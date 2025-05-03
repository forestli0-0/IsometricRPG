// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <InputActionValue.h>
#include "EnhancedInput/Public/InputMappingContext.h" // Add this include
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "IsometricInputComponent.generated.h"

UENUM(BlueprintType)
enum class EInputCommandType : uint8
{
	None,
	BasicAttack,
	UseSkill,
	Movement
};

USTRUCT(BlueprintType)
struct FInputCommand
{
	GENERATED_BODY()

	// 命令类型
	EInputCommandType CommandType = EInputCommandType::None;

	// 目标位置
	FVector TargetLocation;

	// 目标角色
	TWeakObjectPtr<AActor> TargetActor;

	// 技能ID/索引
	int32 SkillIndex = -1;

	// 技能标签
	FGameplayTag AbilityTag;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ISOMETRICRPG_API UIsometricInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UIsometricInputComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	void SetupInput(class UEnhancedInputComponent* InputComponent, class APlayerController* PlayerController);
	// 处理点击事件
	void HandleClick();
	// 处理技能输入事件(1-9号键)
	void HandleSkillInput(int32 SkillIndex);

	// 统一的命令处理函数
	void ProcessInputCommand(const FInputCommand& Command);

	// 获取鼠标下的目标
	AActor* GetTargetUnderCursor() const;

	// 获取鼠标下的位置
	FVector GetLocationUnderCursor() const;
protected:
	// 第一次构建输入系统时，采用的WASD移动，暂时保留。
	void Move(const FInputActionValue& Value);
public:
	// 输入映射上下文
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* MappingContext;
	// 输入动作
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;
	
private:
	// 当前选择的技能索引
	int32 CurrentSelectedSkillIndex = -1;

	// 技能映射 - 可以在编辑器中设置
	UPROPERTY(EditDefaultsOnly, Category = "Input|Skills")
	TMap<int32, FGameplayTag> SkillMappings;
};
