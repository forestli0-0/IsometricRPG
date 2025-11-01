#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Equipment/EquipmentTypes.h"
#include "EquipmentPanelWidget.generated.h"

class UEquipmentComponent;

UCLASS()
class ISOMETRICRPG_API UEquipmentPanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void BindToEquipment(UEquipmentComponent* EquipmentComponent);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UnbindEquipment();

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    UEquipmentComponent* GetEquipmentComponent() const { return BoundEquipment.Get(); }

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Equipment")
    void RefreshEquipment(const TArray<FEquipmentSlotData>& Slots);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Equipment")
    void OnEquipmentSlotUpdated(EEquipmentSlot InSlot, const FEquipmentSlotData& Entry);

protected:
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TWeakObjectPtr<UEquipmentComponent> BoundEquipment;

    UFUNCTION()
    void HandleEquipmentRefreshed(const TArray<FEquipmentSlotData>& Slots);

    UFUNCTION()
    void HandleEquipmentSlotChanged(EEquipmentSlot InSlot, const FEquipmentSlotData& Entry);
};
