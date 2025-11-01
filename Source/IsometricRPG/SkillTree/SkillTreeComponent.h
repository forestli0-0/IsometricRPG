#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillTree/SkillTreeDataAsset.h"
#include "GameplayAbilitySpec.h"
#include "SkillTreeComponent.generated.h"

class AIsoPlayerState;
class UIsometricRPGAttributeSetBase;

USTRUCT(BlueprintType)
struct FSkillTreeNodeState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    bool bUnlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    bool bUnlockable = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    int32 PointsInvested = 0;
};

USTRUCT(BlueprintType)
struct FSkillTreeNodeStateEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    FName NodeId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
    FSkillTreeNodeState State;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSkillTreeNodeStateChangedSignature, FName, NodeId, const FSkillTreeNodeState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillTreeStateRefreshedSignature, const TArray<FSkillTreeNodeStateEntry>&, NewStates);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillTreePointsChangedSignature, int32, AvailablePoints);

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ISOMETRICRPG_API USkillTreeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    USkillTreeComponent();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Tree")
    TObjectPtr<USkillTreeDataAsset> SkillTreeDefinition;

    UPROPERTY(BlueprintAssignable, Category = " Skill Tree|Events")
    FSkillTreeNodeStateChangedSignature OnNodeStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Skill Tree|Events")
    FSkillTreeStateRefreshedSignature OnSkillTreeRefreshed;

    UPROPERTY(BlueprintAssignable, Category = "Skill Tree|Events")
    FSkillTreePointsChangedSignature OnSkillPointChanged;

    UFUNCTION(BlueprintCallable, Category = "Skill Tree")
    bool CanUnlockNode(FName NodeId) const;

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Skill Tree")
    void ServerUnlockNode(FName NodeId);

    UFUNCTION(BlueprintCallable, Category = "Skill Tree")
    const FSkillTreeNodeState& GetNodeStateChecked(FName NodeId) const;

    UFUNCTION(BlueprintCallable, Category = "Skill Tree")
    void RequestRefresh();

    UFUNCTION(BlueprintCallable, Category = "Skill Tree")
    int32 GetAvailableSkillPoints() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnRep_NodeStates(const TArray<FSkillTreeNodeStateEntry>& OldStates);

private:
    // Replicated array of node states
    UPROPERTY(ReplicatedUsing = OnRep_NodeStates)
    TArray<FSkillTreeNodeStateEntry> NodeStateEntries;

    // Transient lookup from NodeId to index in NodeStateEntries for fast access
    UPROPERTY(Transient)
    TMap<FName, int32> NodeIdToIndex;

    TWeakObjectPtr<AIsoPlayerState> CachedPlayerState;
    TWeakObjectPtr<UIsometricRPGAttributeSetBase> BoundAttributeSet;

    UPROPERTY()
    TMap<FName, FGameplayAbilitySpecHandle> GrantedAbilityHandles;

    void InitializeNodeStates();
    void RebuildNodeIndex();

    bool ArePrerequisitesMet(const FSkillTreeNodeDefinition& Definition) const;
    bool ConsumeSkillPoints(int32 Amount);
    void UpdateUnlockableStates();
    void BroadcastFullRefresh();
    void HandleAttributeBindings();
    void CleanupAttributeBindings();
    void UnlockNodeInternal(const FName NodeId, const FSkillTreeNodeDefinition& Definition);
    void GrantAbilityForNode(const FName NodeId, const FSkillTreeNodeDefinition& Definition);
    void RevokeGrantedAbilities();

    void HandleSkillPointAttributeChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue);

    // Helpers to access state entries
    FSkillTreeNodeStateEntry* FindNodeEntryMutable(FName NodeId);
    const FSkillTreeNodeStateEntry* FindNodeEntry(FName NodeId) const;
    FSkillTreeNodeState* FindNodeStateMutable(FName NodeId);
    const FSkillTreeNodeState* FindNodeState(FName NodeId) const;
};


