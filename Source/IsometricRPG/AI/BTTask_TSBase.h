#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"
#include "BTTask_TSBase.generated.h"

/**
 * 专门为 TypeScript 准备的 BTTask 基类
 * 用于解决直接继承 BTTask_BlueprintBase 时，行为树编辑器无法搜索到蓝图的问题
 */
UCLASS(Abstract, Blueprintable)
class ISOMETRICRPG_API UBTTask_TSBase : public UBTTask_BlueprintBase
{
	GENERATED_BODY()
	
};
