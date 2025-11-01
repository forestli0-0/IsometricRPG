#include "UI/EquipmentPanelWidget.h"
//#include "CoreMinimal.h"
#include "Equipment/EquipmentComponent.h"

void UEquipmentPanelWidget::BindToEquipment(UEquipmentComponent* EquipmentComponent)
{
    if (BoundEquipment.Get() == EquipmentComponent)
    {
        return;
    }

    UnbindEquipment();

    if (!EquipmentComponent)
    {
        return;
    }

    BoundEquipment = EquipmentComponent;
    EquipmentComponent->OnEquipmentRefreshed.AddDynamic(this, &UEquipmentPanelWidget::HandleEquipmentRefreshed);
    EquipmentComponent->OnEquipmentSlotChanged.AddDynamic(this, &UEquipmentPanelWidget::HandleEquipmentSlotChanged);

    HandleEquipmentRefreshed(EquipmentComponent->GetEquippedSlots());
}

void UEquipmentPanelWidget::UnbindEquipment()
{
    if (UEquipmentComponent* EquipmentComponent = BoundEquipment.Get())
    {
        EquipmentComponent->OnEquipmentRefreshed.RemoveDynamic(this, &UEquipmentPanelWidget::HandleEquipmentRefreshed);
        EquipmentComponent->OnEquipmentSlotChanged.RemoveDynamic(this, &UEquipmentPanelWidget::HandleEquipmentSlotChanged);
    }

    BoundEquipment = nullptr;
}

void UEquipmentPanelWidget::NativeDestruct()
{
    UnbindEquipment();
    Super::NativeDestruct();
}

void UEquipmentPanelWidget::HandleEquipmentRefreshed(const TArray<FEquipmentSlotData>& Slots)
{
    RefreshEquipment(Slots);
}

void UEquipmentPanelWidget::HandleEquipmentSlotChanged(EEquipmentSlot InSlot, const FEquipmentSlotData& Entry)
{
    OnEquipmentSlotUpdated(InSlot, Entry);
}
