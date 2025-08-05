#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LevelUpData.generated.h"

USTRUCT(BlueprintType)
struct FLevelUpData : public FTableRowBase
{
    GENERATED_BODY()

public:
    // 提升的最大生命值
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MaxHealthGain = 0.0f;

    // 提升的最大法力值
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MaxManaGain = 0.0f;

    // 提升的攻击力
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float AttackDamageGain = 0.0f;

    // 提升的物理防御
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float PhysicalDefenseGain = 0.0f;

    // 提升的魔法防御
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MagicDefenseGain = 0.0f;

    // 升级时获得的技能点
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 SkillPointsAwarded = 0;
};