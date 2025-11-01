#include "Equipment/EquipmentComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Equipment/EquipmentTypes.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemData.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const TMap<FGameplayTag, FGameplayAttribute>& GetEquipmentAttributeMap()
	{
		static TMap<FGameplayTag, FGameplayAttribute> TagToAttribute;
		if (TagToAttribute.Num() == 0)
		{
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Core.MaxHealth")), UIsometricRPGAttributeSetBase::GetMaxHealthAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Core.HealthRegen")), UIsometricRPGAttributeSetBase::GetHealthRegenRateAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Core.MaxMana")), UIsometricRPGAttributeSetBase::GetMaxManaAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Core.ManaRegen")), UIsometricRPGAttributeSetBase::GetManaRegenRateAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Core.MoveSpeed")), UIsometricRPGAttributeSetBase::GetMoveSpeedAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Offense.AttackDamage")), UIsometricRPGAttributeSetBase::GetAttackDamageAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Offense.AttackSpeed")), UIsometricRPGAttributeSetBase::GetAttackSpeedAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Offense.CriticalChance")), UIsometricRPGAttributeSetBase::GetCriticalChanceAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Offense.CriticalDamage")), UIsometricRPGAttributeSetBase::GetCriticalDamageAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Offense.AttackRange")), UIsometricRPGAttributeSetBase::GetAttackRangeAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Offense.ArmorPenetration")), UIsometricRPGAttributeSetBase::GetArmorPenetrationAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Offense.MagicPenetration")), UIsometricRPGAttributeSetBase::GetMagicPenetrationAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Defense.PhysicalDefense")), UIsometricRPGAttributeSetBase::GetPhysicalDefenseAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Defense.MagicDefense")), UIsometricRPGAttributeSetBase::GetMagicDefenseAttribute());
			TagToAttribute.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Defense.LifeSteal")), UIsometricRPGAttributeSetBase::GetLifeStealAttribute());
		}
		return TagToAttribute;
	}
}

UEquipmentComponent::UEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
	InitializeSlots();
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheDependencies();
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEquipmentComponent, EquippedSlots);
}

bool UEquipmentComponent::EquipItemFromInventory(int32 InventorySlotIndex, EEquipmentSlot TargetSlot)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	CacheDependencies();
	if (!CachedInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] No inventory component available for equip."));
		return false;
	}

	if (!CachedInventoryComponent->GetSlots().IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FInventoryItemSlot& SourceSlot = CachedInventoryComponent->GetSlotChecked(InventorySlotIndex);
	if (!SourceSlot.Stack.IsValid())
	{
		return false;
	}

	UInventoryItemData* ItemData = SourceSlot.Stack.Resolve();
	if (!ItemData)
	{
		return false;
	}

	if (ItemData->Category != EInventoryItemCategory::Equipment)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] Attempted to equip non-equipment item %s."), *ItemData->GetName());
		return false;
	}

	return InternalEquip(ItemData, TargetSlot, InventorySlotIndex);
}

bool UEquipmentComponent::EquipItemDirect(UInventoryItemData* ItemData, EEquipmentSlot TargetSlot)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	if (!ItemData || ItemData->Category != EInventoryItemCategory::Equipment)
	{
		return false;
	}

	return InternalEquip(ItemData, TargetSlot, INDEX_NONE);
}

bool UEquipmentComponent::UnequipSlot(EEquipmentSlot Slot, bool bReturnToInventory)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	CacheDependencies();

	const int32 SlotIndex = GetSlotInternalIndex(Slot);
	if (!EquippedSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FEquipmentSlotData& SlotData = EquippedSlots[SlotIndex];
	if (!SlotData.ItemStack.IsValid())
	{
		return false;
	}

	UInventoryItemData* ItemData = SlotData.ItemStack.Resolve();
	const TArray<FEquipmentAppliedModifier> PreviousModifiers = AppliedModifiersBySlot.FindRef(Slot);

	ClearAttributeModifiers(Slot);

	if (bReturnToInventory && CachedInventoryComponent && ItemData)
	{
		const int32 Remaining = CachedInventoryComponent->AddItem(ItemData, 1);
		if (Remaining > 0)
		{
			// rollback attribute removal
			for (const FEquipmentAppliedModifier& Modifier : PreviousModifiers)
			{
				if (CachedAbilitySystemComponent && Modifier.Attribute.IsValid())
				{
					CachedAbilitySystemComponent->ApplyModToAttribute(Modifier.Attribute, EGameplayModOp::Additive, Modifier.Magnitude);
				}
			}
			AppliedModifiersBySlot.Add(Slot, PreviousModifiers);
			return false;
		}
	}

	SlotData.ItemStack.ItemData.Reset();
	SlotData.ItemStack.Quantity = 0;

	AppliedModifiersBySlot.Remove(Slot);

	ForceOwnerNetUpdate();
	OnEquipmentSlotChanged.Broadcast(Slot, SlotData);
	OnEquipmentRefreshed.Broadcast(EquippedSlots);
	return true;
}

FEquipmentSlotData UEquipmentComponent::GetSlotData(EEquipmentSlot Slot) const
{
	const int32 SlotIndex = GetSlotInternalIndex(Slot);
	if (EquippedSlots.IsValidIndex(SlotIndex))
	{
		return EquippedSlots[SlotIndex];
	}
	return FEquipmentSlotData{Slot, FInventoryItemStack()};
}

bool UEquipmentComponent::HasItemEquipped(EEquipmentSlot Slot) const
{
	const int32 SlotIndex = GetSlotInternalIndex(Slot);
	return EquippedSlots.IsValidIndex(SlotIndex) && EquippedSlots[SlotIndex].ItemStack.IsValid();
}

void UEquipmentComponent::OnRep_EquippedSlots(const TArray<FEquipmentSlotData>& OldSlots)
{
	const int32 MaxSlots = FMath::Max(EquippedSlots.Num(), OldSlots.Num());
	for (int32 Index = 0; Index < MaxSlots; ++Index)
	{
		const FEquipmentSlotData* NewSlot = EquippedSlots.IsValidIndex(Index) ? &EquippedSlots[Index] : nullptr;
		const FEquipmentSlotData* OldSlot = OldSlots.IsValidIndex(Index) ? &OldSlots[Index] : nullptr;

		if ((!NewSlot && !OldSlot) || (NewSlot && OldSlot && *NewSlot == *OldSlot))
		{
			continue;
		}

		if (NewSlot)
		{
			OnEquipmentSlotChanged.Broadcast(NewSlot->Slot, *NewSlot);
		}
		else if (OldSlot)
		{
			FEquipmentSlotData Cleared = *OldSlot;
			Cleared.ItemStack.ItemData.Reset();
			Cleared.ItemStack.Quantity = 0;
			OnEquipmentSlotChanged.Broadcast(OldSlot->Slot, Cleared);
		}
	}

	OnEquipmentRefreshed.Broadcast(EquippedSlots);
}

void UEquipmentComponent::InitializeSlots()
{
	const int32 SlotCount = static_cast<int32>(EEquipmentSlot::MAX) - 1;
	EquippedSlots.SetNum(SlotCount);
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		EquippedSlots[Index].Slot = static_cast<EEquipmentSlot>(Index + 1);
		EquippedSlots[Index].ItemStack.Quantity = 0;
		EquippedSlots[Index].ItemStack.ItemData.Reset();
	}
}

int32 UEquipmentComponent::GetSlotInternalIndex(EEquipmentSlot Slot) const
{
	const int32 RawValue = static_cast<int32>(Slot);
	if (RawValue <= static_cast<int32>(EEquipmentSlot::None) || RawValue >= static_cast<int32>(EEquipmentSlot::MAX))
	{
		return INDEX_NONE;
	}
	return RawValue - 1;
}

void UEquipmentComponent::CacheDependencies()
{
	if (!CachedInventoryComponent)
	{
		AActor* OwnerActor = GetOwner();
		if (OwnerActor)
		{
			CachedInventoryComponent = OwnerActor->FindComponentByClass<UInventoryComponent>();
		}
	}

	if (!CachedAbilitySystemComponent)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			if (IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(OwnerActor))
			{
				CachedAbilitySystemComponent = AbilityInterface->GetAbilitySystemComponent();
			}
		}
	}
}

void UEquipmentComponent::ForceOwnerNetUpdate() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

bool UEquipmentComponent::InternalEquip(UInventoryItemData* ItemData, EEquipmentSlot TargetSlot, int32 SourceInventorySlotIndex)
{
	CacheDependencies();

	if (!ItemData)
	{
		return false;
	}

	if (SourceInventorySlotIndex != INDEX_NONE)
	{
		if (!CachedInventoryComponent || !CachedInventoryComponent->RemoveItemAtIndex(SourceInventorySlotIndex, 1))
		{
			return false;
		}
	}

	if (HasItemEquipped(TargetSlot))
	{
		if (!UnequipSlot(TargetSlot, bAutoReturnUnequippedItemsToInventory))
		{
			if (SourceInventorySlotIndex != INDEX_NONE && CachedInventoryComponent)
			{
				CachedInventoryComponent->AddItem(ItemData, 1);
			}
			return false;
		}
	}

	const int32 SlotIndex = GetSlotInternalIndex(TargetSlot);
	if (!EquippedSlots.IsValidIndex(SlotIndex))
	{
		if (SourceInventorySlotIndex != INDEX_NONE && CachedInventoryComponent)
		{
			CachedInventoryComponent->AddItem(ItemData, 1);
		}
		return false;
	}

	FEquipmentSlotData& SlotData = EquippedSlots[SlotIndex];
	SlotData.ItemStack.ItemData = ItemData;
	SlotData.ItemStack.Quantity = 1;

	ApplyAttributeModifiers(TargetSlot, ItemData);

	ForceOwnerNetUpdate();
	OnEquipmentSlotChanged.Broadcast(TargetSlot, SlotData);
	OnEquipmentRefreshed.Broadcast(EquippedSlots);
	return true;
}

void UEquipmentComponent::ApplyAttributeModifiers(EEquipmentSlot Slot, const UInventoryItemData* ItemData)
{
	if (!CachedAbilitySystemComponent || !ItemData)
	{
		return;
	}

	TArray<FEquipmentAppliedModifier>& AppliedModifiers = AppliedModifiersBySlot.FindOrAdd(Slot);
	AppliedModifiers.Reset();

	for (const FInventoryItemStatModifier& Modifier : ItemData->AttributeModifiers)
	{
		FGameplayAttribute Attribute = Modifier.TargetAttribute;
		if (!Attribute.IsValid())
		{
			if (!Modifier.TargetTag.IsValid())
			{
				continue;
			}

			if (!ResolveAttributeFromTag(Modifier.TargetTag, Attribute) || !Attribute.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] Unsupported attribute tag %s on item %s."), *Modifier.TargetTag.ToString(), *ItemData->GetName());
				continue;
			}
		}

		if (!Attribute.IsValid())
		{
			continue;
		}

		CachedAbilitySystemComponent->ApplyModToAttribute(Attribute, EGameplayModOp::Additive, Modifier.Magnitude);
		AppliedModifiers.Emplace(Attribute, Modifier.Magnitude);
	}

	if (AppliedModifiers.Num() == 0)
	{
		AppliedModifiersBySlot.Remove(Slot);
	}
}

void UEquipmentComponent::ClearAttributeModifiers(EEquipmentSlot Slot)
{
	if (!CachedAbilitySystemComponent)
	{
		return;
	}

	if (TArray<FEquipmentAppliedModifier>* Found = AppliedModifiersBySlot.Find(Slot))
	{
		for (const FEquipmentAppliedModifier& Modifier : *Found)
		{
			if (Modifier.Attribute.IsValid())
			{
				CachedAbilitySystemComponent->ApplyModToAttribute(Modifier.Attribute, EGameplayModOp::Additive, -Modifier.Magnitude);
			}
		}
		AppliedModifiersBySlot.Remove(Slot);
	}
}

bool UEquipmentComponent::ResolveAttributeFromTag(const FGameplayTag& Tag, FGameplayAttribute& OutAttribute) const
{
	if (!Tag.IsValid())
	{
		return false;
	}

	const TMap<FGameplayTag, FGameplayAttribute>& TagMap = GetEquipmentAttributeMap();
	if (const FGameplayAttribute* FoundAttribute = TagMap.Find(Tag))
	{
		OutAttribute = *FoundAttribute;
		return OutAttribute.IsValid();
	}

	return false;
}

