#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HUDSkillSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;

/** 轻量视图模型：描述单个技能槽的显示数据（图标、冷却、消耗等）。 */
USTRUCT(BlueprintType)
struct FHUDSkillSlotViewModel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    ESkillSlot Slot = ESkillSlot::Invalid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TSubclassOf<UGameplayAbility> AbilityClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FGameplayTag AbilityPrimaryTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FText HotkeyLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bIsUnlocked = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bIsEquipped = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    float ResourceCost = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    float CooldownDuration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    float CooldownRemaining = 0.f;
};

/**
 * 表示加载栏中的单个技能或动作槽。运行时的视觉/计时逻辑在 C++ 中实现以确保性能。
 */
UCLASS()
class ISOMETRICRPG_API UHUDSkillSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 使用提供的数据填充槽并刷新绑定的子控件显示。 */
    void SetSlotData(const FHUDSkillSlotViewModel& InData);

    /** 清空槽位，标记为空并重置所有视觉元素。 */
    void ClearSlot();

    /** 开始一个冷却计时器，该计时器由 C++ 在 Tick 中驱动并渲染遮罩/文本。 */
    void BeginCooldown(float DurationSeconds, float InitialRemainingSeconds = -1.f);

    /** 如果冷却在进行则中断冷却并隐藏覆盖层。 */
    void CancelCooldown();

    /** 如果当前槽位持有有效数据则返回 true。 */
    bool HasData() const { return bHasData; }

    /** 当冷却遮罩处于激活状态时返回 true。 */
    bool IsOnCooldown() const { return bCooldownActive; }

    /** 配置此控件对应的游戏内槽位（Q/W/E/R/D/F 等）。 */
    void SetConfiguredSlot(ESkillSlot InSlot) { ConfiguredSlot = InSlot; }

    /** 返回当前控件绑定的游戏槽位。 */
    ESkillSlot GetConfiguredSlot() const { return ConfiguredSlot; }

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void ApplySlotDataToWidgets();
    void ApplyCooldownToWidgets(float RemainingSeconds);
    void UpdateEmptyState();
    float GetWorldTime() const;

private:
    /** 槽位的缓存数据模型。 */
    FHUDSkillSlotViewModel SlotData;

    /** 此控件代表的枚举槽（Q/W/E/R/D/F 等）。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot", meta = (AllowPrivateAccess = "true"))
    ESkillSlot ConfiguredSlot = ESkillSlot::Invalid;

    /** 标志：槽位是否持有有效数据。 */
    bool bHasData = false;

    /** 冷却相关的计时字段（以秒为单位）。 */
    bool bCooldownActive = false;
    float CooldownDuration = 0.f;
    float CooldownEndTime = 0.f;

    /** 在 UMG 设计器中配置的子控件，由 C++ 驱动显示内容。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> IconImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> NameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HotkeyText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> CooldownProgress;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> EmptyStateOverlay;

    // 类似 LoL 的圆形冷却遮罩与中心倒计时文本
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> CooldownMaskImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CooldownText;

    // 可选的覆盖材质；若为空则使用 `CooldownMaskImage` 上已设置的材质
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInterface> CooldownRadialMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CooldownMID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (AllowPrivateAccess = "true"))
    FName CooldownPercentParamName = TEXT("Percent");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (AllowPrivateAccess = "true"))
    FLinearColor CooldownMaskTint = FLinearColor(0.f, 0.f, 0.f, 0.65f);

    // 风格：热键标签颜色（例如 Q/W/E/R），可在默认值或蓝图中配置
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
    FSlateColor HotkeyTextColor = FLinearColor::White;
};
