#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillTree/SkillTreeComponent.h"
#include "SkillTreeWidget.generated.h"

UCLASS()
class ISOMETRICRPG_API USkillTreeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Skill Tree")
    void BindSkillTree(USkillTreeComponent* InSkillTree);

    UFUNCTION(BlueprintCallable, Category = "Skill Tree")
    void UnbindSkillTree();

    UFUNCTION(BlueprintCallable, Category = "Skill Tree")
    void RequestUnlockNode(FName NodeId);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Skill Tree")
    void OnNodeStateUpdated(FName NodeId, const FSkillTreeNodeState& NodeState);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Skill Tree")
    void OnSkillTreeDataRefreshed(const TArray<FSkillTreeNodeStateEntry>& NodeStates);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Skill Tree")
    void OnSkillPointsUpdated(int32 RemainingPoints);

protected:
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TWeakObjectPtr<USkillTreeComponent> BoundSkillTree;

    UFUNCTION()
    void HandleNodeStateChanged(FName NodeId, const FSkillTreeNodeState& NewState);

    UFUNCTION()
    void HandleSkillTreeRefreshed(const TArray<FSkillTreeNodeStateEntry>& NodeStates);

    UFUNCTION()
    void HandleSkillPointsChanged(int32 NewValue);
};
