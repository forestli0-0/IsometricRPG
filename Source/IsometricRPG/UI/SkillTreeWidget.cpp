#include "UI/SkillTreeWidget.h"

#include "SkillTree/SkillTreeComponent.h"

void USkillTreeWidget::BindSkillTree(USkillTreeComponent* InSkillTree)
{
    if (BoundSkillTree.Get() == InSkillTree)
    {
        return;
    }

    UnbindSkillTree();

    if (!InSkillTree)
    {
        return;
    }

    BoundSkillTree = InSkillTree;
    InSkillTree->OnNodeStateChanged.AddDynamic(this, &USkillTreeWidget::HandleNodeStateChanged);
    InSkillTree->OnSkillTreeRefreshed.AddDynamic(this, &USkillTreeWidget::HandleSkillTreeRefreshed);
    InSkillTree->OnSkillPointChanged.AddDynamic(this, &USkillTreeWidget::HandleSkillPointsChanged);
    InSkillTree->RequestRefresh();
}

void USkillTreeWidget::UnbindSkillTree()
{
    if (USkillTreeComponent* SkillTree = BoundSkillTree.Get())
    {
        SkillTree->OnNodeStateChanged.RemoveDynamic(this, &USkillTreeWidget::HandleNodeStateChanged);
        SkillTree->OnSkillTreeRefreshed.RemoveDynamic(this, &USkillTreeWidget::HandleSkillTreeRefreshed);
        SkillTree->OnSkillPointChanged.RemoveDynamic(this, &USkillTreeWidget::HandleSkillPointsChanged);
    }

    BoundSkillTree = nullptr;
}

void USkillTreeWidget::RequestUnlockNode(FName NodeId)
{
    if (USkillTreeComponent* SkillTree = BoundSkillTree.Get())
    {
        SkillTree->ServerUnlockNode(NodeId);
    }
}

void USkillTreeWidget::NativeDestruct()
{
    UnbindSkillTree();
    Super::NativeDestruct();
}

void USkillTreeWidget::HandleNodeStateChanged(FName NodeId, const FSkillTreeNodeState& NewState)
{
    OnNodeStateUpdated(NodeId, NewState);
}

void USkillTreeWidget::HandleSkillTreeRefreshed(const TArray<FSkillTreeNodeStateEntry>& NodeStates)
{
    OnSkillTreeDataRefreshed(NodeStates);
}

void USkillTreeWidget::HandleSkillPointsChanged(int32 NewValue)
{
    OnSkillPointsUpdated(NewValue);
}
