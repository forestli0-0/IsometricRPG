#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SkillTreeDataAsset.generated.h"

class UGameplayAbility;

USTRUCT(BlueprintType)
struct FSkillTreeNodeDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    FName NodeId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    TObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree", meta = (ClampMin = "1"))
    int32 Cost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    TArray<FName> PrerequisiteNodeIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    TSubclassOf<UGameplayAbility> AbilityToUnlock;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    FGameplayTagContainer NodeTags;
};

UCLASS(BlueprintType)
class ISOMETRICRPG_API USkillTreeDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    USkillTreeDataAsset();

    static const FPrimaryAssetType SkillTreeAssetType;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    TArray<FSkillTreeNodeDefinition> Nodes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    FName RootNodeId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree", meta = (ClampMin = "1"))
    int32 MaxDepth = 4;

    const FSkillTreeNodeDefinition* FindNodeDefinition(const FName NodeId) const;
};
