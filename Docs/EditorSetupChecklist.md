# 编辑器配置清单（Inventory、Equipment、Loot、Skill Tree、UI）

这份清单把最近的 C++ 改动拆解成“在编辑器里要做的步骤”。自上而下执行，每完成一个里程碑建议编译一次。

## 0）前置条件
- 构建模块：UMG、Slate、SlateCore、GameplayAbilities、GameplayTags、GameplayTasks（已在 `IsometricRPG.Build.cs` 中配置）。
- GameplayTags：如果在装备里使用“Tag → 属性”的映射，确保项目中存在对应标签（例如 `Attributes.Core.MaxHealth` 等）。

## 1）UI Widgets（UMG）
为下列父类创建 Widget Blueprint，变量名要与代码完全一致（`BindWidget` 才能生效）：

- WBP_PlayerHUD（父类：`UPlayerHUDWidget`）
  - 子控件名 `SkillBar`（父类：`USkillBarWidget`；可用已有）
  - 子控件名 `InventoryPanel`（父类：`UInventoryPanelWidget`）
  - 子控件名 `CharacterStatsPanel`（父类：`UCharacterStatsPanelWidget`）
  - 子控件名 `SkillTreePanel`（父类：`USkillTreeWidget`）
  - 子控件名 `SettingsMenu`（父类：`USettingsMenuWidget`）

- WBP_InventoryPanel（父类：`UInventoryPanelWidget`）
  - 需要实现的蓝图事件：
    - `RefreshInventory(Slots)`
    - `OnInventorySlotUpdated(Index, SlotData)`
    - `OnInventoryItemCooldownStarted(ItemId, Duration, EndTime)`
    - `OnInventoryItemCooldownEnded(ItemId)`
    - `OnInventoryCooldownsRefreshed(ActiveCooldowns)`

- WBP_EquipmentPanel（父类：`UEquipmentPanelWidget`）
  - 需要实现的蓝图事件：
    - `RefreshEquipment(Slots)`
    - `OnEquipmentSlotUpdated(Slot, Entry)`

- WBP_SkillTreePanel（父类：`USkillTreeWidget`）
  - 需要实现的蓝图事件：
    - `OnNodeStateUpdated(NodeId, NodeState)`
    - `OnSkillTreeDataRefreshed(NodeStates)`
    - `OnSkillPointsUpdated(RemainingPoints)`
  - 解锁触发：节点点击时调用 `RequestUnlockNode(NodeId)`。

- WBP_SettingsMenu（父类：`USettingsMenuWidget`）
  - 需要实现的蓝图事件：
    - `OnSettingsApplied()`
    - `OnMasterVolumeChanged(NewVolume)`

## 2）GameMode / HUD 绑定
- 确保 PlayerController/Character 创建 HUD（WBP_PlayerHUD），并在 PlayerState 可用时调用 `UPlayerHUDWidget::InitializePanels(PlayerState)`（如在 `OnPossessed`/`OnRep_PlayerState` 时机）。
- 校验 `AIsoPlayerState` 提供以下 Getter：
  - `GetInventoryComponent()`
  - `GetAttributeSet()`
  - `GetSkillTreeComponent()`

## 3）给角色/敌人挂组件
- 玩家（或实现了 `IAbilitySystemInterface` 的拥有者 Pawn）：
  - 添加 `UInventoryComponent`
  - 添加 `UEquipmentComponent`（可选，用于装备 UI）
- 敌人（AI）：
  - 添加 `ULootDropComponent` 并指定 `LootTable`
  - 敌人需具备 ASC 与 `UIsometricRPGAttributeSetBase`，并有生命值变化事件（`OnHealthChanged`）

## 4）Data Asset 资源
- 背包物品（多份）：`UInventoryItemData`
  - 设置 `ItemId`、`DisplayName`、`Category`
  - 若为装备：填写 `AttributeModifiers`（可直接给 `TargetAttribute`，或使用 `TargetTag` 映射）
  - 可选：`GrantedAbility`（用于“使用物品”激活）与 `CooldownDuration`

- 掉落表：`ULootTableDataAsset`
  - 在 `Entries` 中填入 `UInventoryItemData` 的软引用、掉落率、数量范围等

- 技能树：`USkillTreeDataAsset`
  - 设置 `RootNodeId`
  - 填写 `Nodes`：唯一的 `NodeId`、`DisplayName`、`Cost`、`PrerequisiteNodeIds`，可选 `AbilityToUnlock`

## 5）行为树（可选）
- 如使用 TypeScript BT Task：
  - 启用 JsEnv/TypeScript 插件
  - Blackboard 中存在键 `TargetActor` 且由 BT 正确设置
  - 在 BT 里加入 `BTTask_AttackMelee`，它会尝试激活标签 `Ability.Player.MeleeAttack`

## 6）设置菜单资产
- 新建 `SoundMix` 与 `SoundClass` 并赋给 WBP_SettingsMenu
- `SetDisplayGamma` 使用控制台变量 `r.DisplayGamma`；会调用 `ApplyNonResolutionSettings()`

## 7）快速冒烟测试
- 在 PIE（Listen Server）下检查：
  - 背包增/删/移/用实时更新 UI；冷却显示并到时消失
  - 装备/卸下会改变属性（可用调试或 UI 查看）
  - 敌人死亡生成拾取；玩家重叠可拾取；数量耗尽后拾取销毁
  - 技能树解锁遵循前置条件；技能点扣减；UI 同步
  - 设置项即时生效（画质、分辨率/全屏、音量）
