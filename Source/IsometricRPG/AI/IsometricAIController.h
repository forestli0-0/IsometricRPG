#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

class UBehaviorTree;

#include "IsometricAIController.generated.h"

/**
 * IsometricRPG 的基础 AI 控制器类。
 * 包含 AIPerception 等稳定组件，避免 TS 重新编译时蓝图数据丢失。
 */
UCLASS()
class ISOMETRICRPG_API AIsometricAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AIsometricAIController();

protected:
	// AI 感知组件——在 C++ 中定义以保证稳定性
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<class UAIPerceptionComponent> AIPerceptionComponent;

	// 行为树资源引用，方便蓝图设置而不依赖脚本定义的成员
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
