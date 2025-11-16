#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDSkillLoadoutWidget.generated.h"

struct FHUDSkillSlotViewModel;
class UHUDSkillSlotWidget;

/**
 * Container widget responsible for arranging the QWER skill slots and auxiliary action items.
 */
UCLASS()
class ISOMETRICRPG_API UHUDSkillLoadoutWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Registers a slot (called from UMG designer to wire slot widgets). */
    UFUNCTION(BlueprintCallable, Category = "SkillLoadout")
    void RegisterSlot(UHUDSkillSlotWidget* InSlot);

    /** Removes every slot reference, typically before rebuilding via designer. */
    UFUNCTION(BlueprintCallable, Category = "SkillLoadout")
    void ResetSlots();

    /** Assigns view data to a specific slot index. */
    void AssignSlot(int32 Index, const FHUDSkillSlotViewModel& ViewModel);

    /** Clears the slot at the specified index. */
    void ClearSlot(int32 Index);

    /** Returns immutable access to the registered slots. */
    const TArray<TObjectPtr<UHUDSkillSlotWidget>>& GetSlots() const { return SkillSlots; }

protected:
    UPROPERTY(VisibleAnywhere, Category = "SkillLoadout")
    TArray<TObjectPtr<UHUDSkillSlotWidget>> SkillSlots;
};
