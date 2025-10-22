#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterStatsPanelWidget.generated.h"

class UIsometricRPGAttributeSetBase;

USTRUCT(BlueprintType)
struct FCharacterStatView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FName StatName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float CurrentValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxValue = 0.f;
};

UCLASS()
class ISOMETRICRPG_API UCharacterStatsPanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void InitializeWithAttributeSet(UIsometricRPGAttributeSetBase* InAttributeSet);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ClearBindings();

    UFUNCTION(BlueprintCallable, Category = "Stats")
    TArray<FCharacterStatView> GetCurrentStats() const { return CachedStats; }

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Stats")
    void RefreshStatList(const TArray<FCharacterStatView>& Stats);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Stats")
    void OnStatChanged(FName StatName, const FCharacterStatView& NewValues);

protected:
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TObjectPtr<UIsometricRPGAttributeSetBase> BoundAttributeSet;

    UPROPERTY()
    TArray<FCharacterStatView> CachedStats;

    UPROPERTY()
    TMap<FName, int32> StatIndexLookup;

    void RebuildStats();
    void BindDelegates();
    void UpdateStatValue(FName StatName, float CurrentValue, float MaxValue = 0.f, bool bBroadcast = true);

    UFUNCTION()
    void HandleHealthChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue);

    UFUNCTION()
    void HandleManaChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue);

    UFUNCTION()
    void HandleExperienceChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue, float NewMaxValue);

    UFUNCTION()
    void HandleLevelChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewLevel);

    UFUNCTION()
    void HandleSkillPointChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue);
};
