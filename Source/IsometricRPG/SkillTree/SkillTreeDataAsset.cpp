#include "SkillTree/SkillTreeDataAsset.h"

const FPrimaryAssetType USkillTreeDataAsset::SkillTreeAssetType(TEXT("SkillTree"));

USkillTreeDataAsset::USkillTreeDataAsset()
{
}

FPrimaryAssetId USkillTreeDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(SkillTreeAssetType, GetFName());
}

const FSkillTreeNodeDefinition* USkillTreeDataAsset::FindNodeDefinition(const FName NodeId) const
{
    return Nodes.FindByPredicate([&](const FSkillTreeNodeDefinition& Node)
    {
        return Node.NodeId == NodeId;
    });
}
