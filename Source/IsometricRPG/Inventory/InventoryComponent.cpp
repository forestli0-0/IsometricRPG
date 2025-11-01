#include "Inventory/InventoryComponent.h"

#include "Inventory/InventoryItemData.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UInventoryComponent::UInventoryComponent()
{
    SetIsReplicatedByDefault(true);
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureCapacity();
    RebuildCooldownIndex();
}

void UInventoryComponent::EnsureCapacity()
{
    if (Capacity < 0)
    {
        Capacity = 0;
    }

    const int32 PreviousCount = Slots.Num();
    Slots.SetNum(Capacity);
    for (int32 Index = 0; Index < Slots.Num(); ++Index)
    {
        Slots[Index].Index = Index;
        if (Slots[Index].Stack.Quantity <= 0)
        {
            Slots[Index].Stack.ItemData.Reset();
            Slots[Index].Stack.Quantity = 0;
        }
    }

    if (PreviousCount != Slots.Num() && GetOwnerRole() == ROLE_Authority)
    {
        BroadcastRefresh();
    }
}

int32 UInventoryComponent::AddItem(UInventoryItemData* ItemData, int32 Quantity)
{
    if (!ItemData || Quantity <= 0)
    {
        return Quantity;
    }

    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] AddItem should be called on the authority only."));
        return Quantity;
    }

    EnsureCapacity();

    int32 Remaining = Quantity;
    if (ItemData->IsStackable())
    {
        Remaining = AddToExistingStacks(ItemData, Remaining);
    }

    if (Remaining > 0)
    {
        Remaining = AddToEmptySlots(ItemData, Remaining);
    }

    if (Remaining != Quantity)
    {
        BroadcastRefresh();
    }

    return Remaining;
}

bool UInventoryComponent::RemoveItemById(FName ItemId, int32 Quantity)
{
    if (Quantity <= 0)
    {
        return false;
    }

    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] RemoveItemById should be called on the authority only."));
        return false;
    }

    int32 Remaining = Quantity;
    for (FInventoryItemSlot& Slot : Slots)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (!Slot.Stack.IsValid())
        {
            continue;
        }

        UInventoryItemData* ItemData = Slot.Stack.Resolve();
        if (!ItemData || ItemData->ItemId != ItemId)
        {
            continue;
        }

        const int32 RemoveCount = FMath::Min(Slot.Stack.Quantity, Remaining);
        Slot.Stack.Quantity -= RemoveCount;
        Remaining -= RemoveCount;

        if (Slot.Stack.Quantity <= 0)
        {
            Slot.Stack.ItemData.Reset();
            Slot.Stack.Quantity = 0;
        }

        BroadcastSlotChanged(Slot.Index);
    }

    if (Remaining < Quantity)
    {
        BroadcastRefresh();
    }

    return Remaining <= 0;
}

bool UInventoryComponent::RemoveItemAtIndex(int32 SlotIndex, int32 Quantity)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] RemoveItemAtIndex should be called on the authority only."));
        return false;
    }

    if (!Slots.IsValidIndex(SlotIndex) || Quantity <= 0)
    {
        return false;
    }

    FInventoryItemSlot& Slot = Slots[SlotIndex];
    if (!Slot.Stack.IsValid())
    {
        return false;
    }

    const int32 RemoveCount = FMath::Min(Slot.Stack.Quantity, Quantity);
    Slot.Stack.Quantity -= RemoveCount;

    if (Slot.Stack.Quantity <= 0)
    {
        Slot.Stack.ItemData.Reset();
        Slot.Stack.Quantity = 0;
    }

    BroadcastSlotChanged(Slot.Index);
    BroadcastRefresh();
    return RemoveCount > 0;
}

bool UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] MoveItem should be called on the authority only."));
        return false;
    }

    if (!Slots.IsValidIndex(FromIndex) || !Slots.IsValidIndex(ToIndex) || FromIndex == ToIndex)
    {
        return false;
    }

    FInventoryItemSlot& FromSlot = Slots[FromIndex];
    FInventoryItemSlot& ToSlot = Slots[ToIndex];

    if (!FromSlot.Stack.IsValid())
    {
        return false;
    }

    if (!ToSlot.Stack.IsValid())
    {
        ToSlot.Stack = FromSlot.Stack;
        FromSlot.Stack.ItemData.Reset();
        FromSlot.Stack.Quantity = 0;
    }
    else
    {
        UInventoryItemData* FromData = FromSlot.Stack.Resolve();
        UInventoryItemData* ToData = ToSlot.Stack.Resolve();

        if (FromData && ToData && FromData == ToData)
        {
            const int32 MaxStack = FromData->MaxStackSize;
            const int32 Space = MaxStack - ToSlot.Stack.Quantity;
            if (Space <= 0)
            {
                return false;
            }

            const int32 Transfer = FMath::Min(Space, FromSlot.Stack.Quantity);
            ToSlot.Stack.Quantity += Transfer;
            FromSlot.Stack.Quantity -= Transfer;
            if (FromSlot.Stack.Quantity <= 0)
            {
                FromSlot.Stack.ItemData.Reset();
                FromSlot.Stack.Quantity = 0;
            }
        }
        else
        {
            Swap(FromSlot.Stack, ToSlot.Stack);
        }
    }

    BroadcastSlotChanged(FromIndex);
    BroadcastSlotChanged(ToIndex);
    BroadcastRefresh();
    return true;
}

int32 UInventoryComponent::CountItem(FName ItemId) const
{
    int32 Total = 0;
    for (const FInventoryItemSlot& Slot : Slots)
    {
        if (!Slot.Stack.IsValid())
        {
            continue;
        }

        const UInventoryItemData* ItemData = Slot.Stack.Resolve();
        if (ItemData && ItemData->ItemId == ItemId)
        {
            Total += Slot.Stack.Quantity;
        }
    }
    return Total;
}

bool UInventoryComponent::UseItemAtIndex(int32 SlotIndex)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] UseItemAtIndex should be called on the authority only."));
        return false;
    }

    if (!Slots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    FInventoryItemSlot& Slot = Slots[SlotIndex];
    if (!Slot.Stack.IsValid())
    {
        return false;
    }

    UInventoryItemData* ItemData = nullptr;
    if (Slot.Stack.ItemData.IsValid())
    {
        ItemData = Slot.Stack.ItemData.Get();
    }
    else if (!Slot.Stack.ItemData.IsNull())
    {
        ItemData = Slot.Stack.ItemData.LoadSynchronous();
    }

    if (!ItemData)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] Failed to resolve item data while using slot %d"), SlotIndex);
        return false;
    }

    const FName ItemId = ItemData->ItemId;

    if (!ItemId.IsNone() && IsItemOnCooldown(ItemId))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[InventoryComponent] Item %s is still on cooldown."), *ItemId.ToString());
        return false;
    }

    if (!TryApplyItemUsageEffects(ItemData))
    {
        return false;
    }

    if (ItemData->bConsumeOnUse)
    {
        Slot.Stack.Quantity = FMath::Max(0, Slot.Stack.Quantity - 1);
        if (Slot.Stack.Quantity <= 0)
        {
            Slot.Stack.ItemData.Reset();
            Slot.Stack.Quantity = 0;
        }
    }

    BroadcastSlotChanged(SlotIndex);
    BroadcastRefresh();

    if (!ItemId.IsNone() && ItemData->CooldownDuration > 0.f)
    {
        StartItemCooldownInternal(ItemId, ItemData->CooldownDuration);
    }

    OnInventoryItemUsed.Broadcast(SlotIndex, Slots[SlotIndex]);
    return true;
}

const FInventoryItemSlot& UInventoryComponent::GetSlotChecked(int32 SlotIndex) const
{
    check(Slots.IsValidIndex(SlotIndex));
    return Slots[SlotIndex];
}

void UInventoryComponent::OnRep_Slots(const TArray<FInventoryItemSlot>& OldSlots)
{
    const int32 MaxIndex = FMath::Max(Slots.Num(), OldSlots.Num());
    for (int32 Index = 0; Index < MaxIndex; ++Index)
    {
        const FInventoryItemSlot* NewSlot = Slots.IsValidIndex(Index) ? &Slots[Index] : nullptr;
        const FInventoryItemSlot* OldSlot = OldSlots.IsValidIndex(Index) ? &OldSlots[Index] : nullptr;

        if ((!NewSlot && !OldSlot) || (NewSlot && OldSlot && SlotsEqual(*NewSlot, *OldSlot)))
        {
            continue;
        }

        FInventoryItemSlot Payload;
        Payload.Index = Index;
        if (NewSlot)
        {
            Payload = *NewSlot;
        }

        const bool bStackDecreased =
            (NewSlot && OldSlot &&
                NewSlot->Stack.ItemData.ToSoftObjectPath() == OldSlot->Stack.ItemData.ToSoftObjectPath() &&
                NewSlot->Stack.Quantity < OldSlot->Stack.Quantity) ||
            (!NewSlot && OldSlot && OldSlot->Stack.IsValid());

        if (bStackDecreased)
        {
            OnInventoryItemUsed.Broadcast(Index, Payload);
        }

        OnInventorySlotChanged.Broadcast(Index, Payload);
    }

    OnInventoryRefreshed.Broadcast(Slots);
}

int32 UInventoryComponent::AddToExistingStacks(UInventoryItemData* ItemData, int32 Quantity)
{
    int32 Remaining = Quantity;
    for (FInventoryItemSlot& Slot : Slots)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (!Slot.Stack.IsValid())
        {
            continue;
        }

        UInventoryItemData* ExistingData = Slot.Stack.Resolve();
        if (ExistingData != ItemData)
        {
            continue;
        }

        const int32 MaxStack = ItemData->MaxStackSize;
        const int32 Space = MaxStack - Slot.Stack.Quantity;
        if (Space <= 0)
        {
            continue;
        }

        const int32 Added = FMath::Min(Space, Remaining);
        Slot.Stack.Quantity += Added;
        Remaining -= Added;
        BroadcastSlotChanged(Slot.Index);
    }
    return Remaining;
}

int32 UInventoryComponent::AddToEmptySlots(UInventoryItemData* ItemData, int32 Quantity)
{
    int32 Remaining = Quantity;
    for (FInventoryItemSlot& Slot : Slots)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (Slot.Stack.IsValid())
        {
            continue;
        }

        const int32 MaxStack = ItemData->IsStackable() ? ItemData->MaxStackSize : 1;
        const int32 Added = FMath::Min(MaxStack, Remaining);
        Slot.Stack.ItemData = ItemData;
        Slot.Stack.Quantity = Added;
        Remaining -= Added;
        BroadcastSlotChanged(Slot.Index);
    }
    return Remaining;
}

void UInventoryComponent::BroadcastSlotChanged(int32 SlotIndex)
{
    if (Slots.IsValidIndex(SlotIndex))
    {
        OnInventorySlotChanged.Broadcast(SlotIndex, Slots[SlotIndex]);
    }
}

void UInventoryComponent::BroadcastRefresh()
{
    OnInventoryRefreshed.Broadcast(Slots);
}

bool UInventoryComponent::SlotsEqual(const FInventoryItemSlot& A, const FInventoryItemSlot& B)
{
    return A.Index == B.Index && A.Stack == B.Stack;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UInventoryComponent, Slots);
    DOREPLIFETIME(UInventoryComponent, ActiveCooldowns);
}

bool UInventoryComponent::IsItemOnCooldown(FName ItemId) const
{
    if (ItemId.IsNone())
    {
        return false;
    }

    const FInventoryItemCooldown* Entry = FindCooldownEntry(ItemId);
    if (!Entry)
    {
        return false;
    }

    if (const UWorld* World = GetWorld())
    {
        const float Remaining = Entry->CooldownEndTime - World->GetTimeSeconds();
        return Remaining > KINDA_SMALL_NUMBER;
    }

    return true;
}

float UInventoryComponent::GetRemainingCooldown(FName ItemId) const
{
    if (ItemId.IsNone())
    {
        return 0.f;
    }

    const FInventoryItemCooldown* Entry = FindCooldownEntry(ItemId);
    if (!Entry)
    {
        return 0.f;
    }

    if (const UWorld* World = GetWorld())
    {
        return FMath::Max(0.f, Entry->CooldownEndTime - World->GetTimeSeconds());
    }

    return Entry->Duration;
}

void UInventoryComponent::OnRep_ActiveCooldowns(const TArray<FInventoryItemCooldown>& OldCooldowns)
{
    TMap<FName, FInventoryItemCooldown> OldLookup;
    for (const FInventoryItemCooldown& Entry : OldCooldowns)
    {
        OldLookup.Add(Entry.ItemId, Entry);
    }

    for (const FInventoryItemCooldown& Entry : ActiveCooldowns)
    {
        FInventoryItemCooldown* OldEntry = OldLookup.Find(Entry.ItemId);
        if (!OldEntry || *OldEntry != Entry)
        {
            OnInventoryItemCooldownStarted.Broadcast(Entry.ItemId, Entry.Duration, Entry.CooldownEndTime);
        }
    }

    for (const FInventoryItemCooldown& Entry : OldCooldowns)
    {
        if (!FindCooldownEntry(Entry.ItemId))
        {
            OnInventoryItemCooldownEnded.Broadcast(Entry.ItemId);
        }
    }

    RebuildCooldownIndex();
    OnInventoryCooldownsRefreshed.Broadcast(ActiveCooldowns);
}

bool UInventoryComponent::TryApplyItemUsageEffects(UInventoryItemData* ItemData)
{
    if (!ItemData)
    {
        return false;
    }

    IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(GetOwner());
    UAbilitySystemComponent* AbilitySystem = AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr;

    if (AbilitySystem && ItemData->GrantedAbility)
    {
        const bool bActivated = AbilitySystem->TryActivateAbilityByClass(ItemData->GrantedAbility, true);
        if (!bActivated)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] Failed to activate ability %s from item %s"),
                *ItemData->GrantedAbility->GetName(), *ItemData->GetName());
            return false;
        }
    }

    return true;
}

void UInventoryComponent::StartItemCooldownInternal(FName ItemId, float Duration)
{
    if (GetOwnerRole() != ROLE_Authority || ItemId.IsNone() || Duration <= 0.f)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float EndTime = World->GetTimeSeconds() + Duration;

    if (FInventoryItemCooldown* Existing = FindCooldownEntryMutable(ItemId))
    {
        Existing->Duration = Duration;
        Existing->CooldownEndTime = EndTime;
    }
    else
    {
        FInventoryItemCooldown NewEntry;
        NewEntry.ItemId = ItemId;
        NewEntry.Duration = Duration;
        NewEntry.CooldownEndTime = EndTime;
        ActiveCooldowns.Add(NewEntry);
    }

    RebuildCooldownIndex();

    FTimerHandle& Handle = ActiveCooldownHandles.FindOrAdd(ItemId);
    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &UInventoryComponent::HandleCooldownExpired, ItemId);

    World->GetTimerManager().ClearTimer(Handle);
    World->GetTimerManager().SetTimer(Handle, Delegate, Duration, false);

    OnInventoryItemCooldownStarted.Broadcast(ItemId, Duration, EndTime);
    OnInventoryCooldownsRefreshed.Broadcast(ActiveCooldowns);

    ForceOwnerNetUpdate();
}

void UInventoryComponent::HandleCooldownExpired(FName ItemId)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        if (FTimerHandle* Handle = ActiveCooldownHandles.Find(ItemId))
        {
            World->GetTimerManager().ClearTimer(*Handle);
        }
    }

    ActiveCooldownHandles.Remove(ItemId);

    const int32 Index = FindCooldownIndex(ItemId);
    if (ActiveCooldowns.IsValidIndex(Index))
    {
        ActiveCooldowns.RemoveAtSwap(Index);
        RebuildCooldownIndex();
        ForceOwnerNetUpdate();
    }

    OnInventoryItemCooldownEnded.Broadcast(ItemId);
    OnInventoryCooldownsRefreshed.Broadcast(ActiveCooldowns);
}

void UInventoryComponent::RebuildCooldownIndex()
{
    CooldownIndexLookup.Empty();
    for (int32 Index = 0; Index < ActiveCooldowns.Num(); ++Index)
    {
        CooldownIndexLookup.Add(ActiveCooldowns[Index].ItemId, Index);
    }
}

int32 UInventoryComponent::FindCooldownIndex(FName ItemId) const
{
    if (ItemId.IsNone())
    {
        return INDEX_NONE;
    }

    if (const int32* FoundIndex = CooldownIndexLookup.Find(ItemId))
    {
        if (ActiveCooldowns.IsValidIndex(*FoundIndex) && ActiveCooldowns[*FoundIndex].ItemId == ItemId)
        {
            return *FoundIndex;
        }
    }

    for (int32 Index = 0; Index < ActiveCooldowns.Num(); ++Index)
    {
        if (ActiveCooldowns[Index].ItemId == ItemId)
        {
            return Index;
        }
    }

    return INDEX_NONE;
}

FInventoryItemCooldown* UInventoryComponent::FindCooldownEntryMutable(FName ItemId)
{
    const int32 Index = FindCooldownIndex(ItemId);
    return ActiveCooldowns.IsValidIndex(Index) ? &ActiveCooldowns[Index] : nullptr;
}

const FInventoryItemCooldown* UInventoryComponent::FindCooldownEntry(FName ItemId) const
{
    const int32 Index = FindCooldownIndex(ItemId);
    return ActiveCooldowns.IsValidIndex(Index) ? &ActiveCooldowns[Index] : nullptr;
}

void UInventoryComponent::ForceOwnerNetUpdate() const
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->ForceNetUpdate();
    }
}
