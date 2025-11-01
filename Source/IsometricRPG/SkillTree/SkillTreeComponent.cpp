#include "SkillTree/SkillTreeComponent.h"

#include "AbilitySystemComponent.h"
#include "Character/IsoPlayerState.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Net/UnrealNetwork.h"

USkillTreeComponent::USkillTreeComponent()
{
    SetIsReplicatedByDefault(true);
}

void USkillTreeComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedPlayerState = Cast<AIsoPlayerState>(GetOwner());

    if (GetOwnerRole() == ROLE_Authority)
    {
        InitializeNodeStates();
    }

    // Build lookup for clients in case we already have replicated data
    RebuildNodeIndex();

    HandleAttributeBindings();
    BroadcastFullRefresh();
}

void USkillTreeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        RevokeGrantedAbilities();
    }

    CleanupAttributeBindings();
    Super::EndPlay(EndPlayReason);
}

void USkillTreeComponent::InitializeNodeStates()
{
    NodeStateEntries.Empty();

    if (!SkillTreeDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SkillTreeComponent] SkillTreeDefinition is not set on %s"), *GetOwner()->GetName());
        return;
    }

    for (const FSkillTreeNodeDefinition& Node : SkillTreeDefinition->Nodes)
    {
        FSkillTreeNodeStateEntry Entry;
        Entry.NodeId = Node.NodeId;
        Entry.State = FSkillTreeNodeState();
        NodeStateEntries.Add(Entry);
    }

    RebuildNodeIndex();
    UpdateUnlockableStates();
}

void USkillTreeComponent::RebuildNodeIndex()
{
    NodeIdToIndex.Empty(NodeStateEntries.Num());
    for (int32 Index = 0; Index < NodeStateEntries.Num(); ++Index)
    {
        NodeIdToIndex.Add(NodeStateEntries[Index].NodeId, Index);
    }
}

bool USkillTreeComponent::ArePrerequisitesMet(const FSkillTreeNodeDefinition& Definition) const
{
    if (Definition.PrerequisiteNodeIds.Num() == 0)
    {
        return true;
    }

    for (const FName& Dependency : Definition.PrerequisiteNodeIds)
    {
        const FSkillTreeNodeState* DependencyState = FindNodeState(Dependency);
        if (!DependencyState || !DependencyState->bUnlocked)
        {
            return false;
        }
    }

    return true;
}

bool USkillTreeComponent::CanUnlockNode(FName NodeId) const
{
    if (!SkillTreeDefinition)
    {
        return false;
    }

    const FSkillTreeNodeDefinition* Definition = SkillTreeDefinition->FindNodeDefinition(NodeId);
    if (!Definition)
    {
        return false;
    }

    const FSkillTreeNodeState* ExistingState = FindNodeState(NodeId);
    if (!ExistingState || ExistingState->bUnlocked)
    {
        return false;
    }

    if (!ArePrerequisitesMet(*Definition))
    {
        return false;
    }

    return GetAvailableSkillPoints() >= Definition->Cost;
}

void USkillTreeComponent::ServerUnlockNode_Implementation(FName NodeId)
{
    if (!SkillTreeDefinition)
    {
        return;
    }

    const FSkillTreeNodeDefinition* Definition = SkillTreeDefinition->FindNodeDefinition(NodeId);
    if (!Definition)
    {
        return;
    }

    if (!CanUnlockNode(NodeId))
    {
        return;
    }

    if (!ConsumeSkillPoints(Definition->Cost))
    {
        return;
    }

    UnlockNodeInternal(NodeId, *Definition);
}

void USkillTreeComponent::UnlockNodeInternal(const FName NodeId, const FSkillTreeNodeDefinition& Definition)
{
    if (FSkillTreeNodeState* State = FindNodeStateMutable(NodeId))
    {
        State->bUnlocked = true;
        State->bUnlockable = false;
        State->PointsInvested += Definition.Cost;
    }

    GrantAbilityForNode(NodeId, Definition);

    UpdateUnlockableStates();
    BroadcastFullRefresh();
}

void USkillTreeComponent::UpdateUnlockableStates()
{
    if (!SkillTreeDefinition)
    {
        return;
    }

    for (const FSkillTreeNodeDefinition& Node : SkillTreeDefinition->Nodes)
    {
        if (FSkillTreeNodeState* State = FindNodeStateMutable(Node.NodeId))
        {
            const bool bCanUnlock = !State->bUnlocked && ArePrerequisitesMet(Node) && GetAvailableSkillPoints() >= Node.Cost;
            State->bUnlockable = bCanUnlock;
        }
    }
}

bool USkillTreeComponent::ConsumeSkillPoints(int32 Amount)
{
    if (Amount <= 0)
    {
        return true;
    }

    AIsoPlayerState* PlayerState = CachedPlayerState.Get();
    if (!PlayerState)
    {
        return false;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    UIsometricRPGAttributeSetBase* AttributeSet = PlayerState->GetAttributeSet();
    if (!ASC || !AttributeSet)
    {
        return false;
    }

    const float CurrentPoints = AttributeSet->GetUnUsedSkillPoint();
    if (CurrentPoints < Amount)
    {
        return false;
    }

    ASC->ApplyModToAttribute(UIsometricRPGAttributeSetBase::GetUnUsedSkillPointAttribute(), EGameplayModOp::Additive, -Amount);
    return true;
}

void USkillTreeComponent::BroadcastFullRefresh()
{
    UpdateUnlockableStates();

    OnSkillTreeRefreshed.Broadcast(NodeStateEntries);

    for (const FSkillTreeNodeStateEntry& Entry : NodeStateEntries)
    {
        OnNodeStateChanged.Broadcast(Entry.NodeId, Entry.State);
    }

    OnSkillPointChanged.Broadcast(GetAvailableSkillPoints());
}

void USkillTreeComponent::HandleAttributeBindings()
{
    CleanupAttributeBindings();

    AIsoPlayerState* PlayerState = CachedPlayerState.Get();
    if (!PlayerState)
    {
        return;
    }

    if (UIsometricRPGAttributeSetBase* AttributeSet = PlayerState->GetAttributeSet())
    {
        BoundAttributeSet = AttributeSet;
        AttributeSet->OnUnUsedSkillPointChanged.AddDynamic(this, &USkillTreeComponent::HandleSkillPointAttributeChanged);
        AttributeSet->OnTotalSkillPointChanged.AddDynamic(this, &USkillTreeComponent::HandleSkillPointAttributeChanged);
    }
}

void USkillTreeComponent::HandleSkillPointAttributeChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue)
{
    BroadcastFullRefresh();
}

int32 USkillTreeComponent::GetAvailableSkillPoints() const
{
    const AIsoPlayerState* PlayerState = CachedPlayerState.Get();
    if (!PlayerState)
    {
        return 0;
    }

    const UIsometricRPGAttributeSetBase* AttributeSet = PlayerState->GetAttributeSet();
    if (!AttributeSet)
    {
        return 0;
    }

    return FMath::FloorToInt(AttributeSet->GetUnUsedSkillPoint());
}

const FSkillTreeNodeState& USkillTreeComponent::GetNodeStateChecked(FName NodeId) const
{
    for (const FSkillTreeNodeStateEntry& Entry : NodeStateEntries)
    {
        if (Entry.NodeId == NodeId)
        {
            return Entry.State;
        }
    }
    static FSkillTreeNodeState DummyState;
    return DummyState;
}

void USkillTreeComponent::RequestRefresh()
{
    BroadcastFullRefresh();
}

void USkillTreeComponent::OnRep_NodeStates(const TArray<FSkillTreeNodeStateEntry>& OldStates)
{
    RebuildNodeIndex();
    OnSkillTreeRefreshed.Broadcast(NodeStateEntries);
}

void USkillTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USkillTreeComponent, NodeStateEntries);
}

void USkillTreeComponent::CleanupAttributeBindings()
{
    if (UIsometricRPGAttributeSetBase* AttributeSet = BoundAttributeSet.Get())
    {
        AttributeSet->OnUnUsedSkillPointChanged.RemoveDynamic(this, &USkillTreeComponent::HandleSkillPointAttributeChanged);
        AttributeSet->OnTotalSkillPointChanged.RemoveDynamic(this, &USkillTreeComponent::HandleSkillPointAttributeChanged);
    }

    BoundAttributeSet = nullptr;
}

void USkillTreeComponent::GrantAbilityForNode(const FName NodeId, const FSkillTreeNodeDefinition& Definition)
{
    if (GetOwnerRole() != ROLE_Authority || !Definition.AbilityToUnlock)
    {
        return;
    }

    if (const FGameplayAbilitySpecHandle* ExistingHandle = GrantedAbilityHandles.Find(NodeId))
    {
        if (ExistingHandle->IsValid())
        {
            return;
        }
    }

    AIsoPlayerState* PlayerState = CachedPlayerState.Get();
    if (!PlayerState)
    {
        return;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    FGameplayAbilitySpec Spec(Definition.AbilityToUnlock, 1, INDEX_NONE, this);
    const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
    GrantedAbilityHandles.Add(NodeId, Handle);
}

void USkillTreeComponent::RevokeGrantedAbilities()
{
    AIsoPlayerState* PlayerState = CachedPlayerState.Get();
    if (!PlayerState)
    {
        GrantedAbilityHandles.Empty();
        return;
    }

    if (UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent())
    {
        for (TPair<FName, FGameplayAbilitySpecHandle>& Pair : GrantedAbilityHandles)
        {
            if (Pair.Value.IsValid())
            {
                ASC->ClearAbility(Pair.Value);
            }
        }
    }

    GrantedAbilityHandles.Empty();
}

// Helpers
FSkillTreeNodeStateEntry* USkillTreeComponent::FindNodeEntryMutable(FName NodeId)
{
    if (const int32* IndexPtr = NodeIdToIndex.Find(NodeId))
    {
        const int32 Index = *IndexPtr;
        if (NodeStateEntries.IsValidIndex(Index))
        {
            return &NodeStateEntries[Index];
        }
    }
    // Fallback to linear search if index not found (e.g., not initialized yet)
    for (FSkillTreeNodeStateEntry& Entry : NodeStateEntries)
    {
        if (Entry.NodeId == NodeId)
        {
            return &Entry;
        }
    }
    return nullptr;
}

const FSkillTreeNodeStateEntry* USkillTreeComponent::FindNodeEntry(FName NodeId) const
{
    if (const int32* IndexPtr = NodeIdToIndex.Find(NodeId))
    {
        const int32 Index = *IndexPtr;
        if (NodeStateEntries.IsValidIndex(Index))
        {
            return &NodeStateEntries[Index];
        }
    }
    for (const FSkillTreeNodeStateEntry& Entry : NodeStateEntries)
    {
        if (Entry.NodeId == NodeId)
        {
            return &Entry;
        }
    }
    return nullptr;
}

FSkillTreeNodeState* USkillTreeComponent::FindNodeStateMutable(FName NodeId)
{
    if (FSkillTreeNodeStateEntry* Entry = FindNodeEntryMutable(NodeId))
    {
        return &Entry->State;
    }
    return nullptr;
}

const FSkillTreeNodeState* USkillTreeComponent::FindNodeState(FName NodeId) const
{
    if (const FSkillTreeNodeStateEntry* Entry = FindNodeEntry(NodeId))
    {
        return &Entry->State;
    }
    return nullptr;
}
