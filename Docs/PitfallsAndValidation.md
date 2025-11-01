# 易踩坑与验证

这是一份精炼的“常见坑 + 可操作检查”的清单，帮助你用最少步骤拿到把握。

## 易踩坑（需要留意什么）

1）仅服务器可执行的操作
- 背包增/删/用、装备的穿/脱、掉落生成都在服务器执行。
- 现象：仅客户端 PIE 下 UI 不更新。解决：使用 Listen Server 启动，或确保逻辑由服务器端发起。

2）`BindWidget` 名称不一致
- `UPlayerHUDWidget` 中变量名必须与子控件名称一字不差：`SkillBar`、`InventoryPanel`、`CharacterStatsPanel`、`SkillTreePanel`、`SettingsMenu`。

3）GameplayTag → 属性映射
- 装备支持通过 `TargetTag` 按标签映射到属性，如 `Attributes.Core.MaxHealth`。
- 缺少标签会导致警告且无效果。可在物品数据里直接填 `TargetAttribute`，或在项目设置中声明标签并在 `UIsometricRPGAttributeSetBase` 中支持映射。

4）能力系统存在性
- `UEquipmentComponent` 通过 ASC 应用属性加成；Owner 必须实现 `IAbilitySystemInterface` 并且实例化了 ASC。
- `ULootDropComponent` 依赖 `UIsometricRPGAttributeSetBase::OnHealthChanged`。

5）软引用未加载
- 背包物品、掉落表条目是 `TSoftObjectPtr`；运行期按需加载。确保引用路径存在且资产可用。

6）技能点事件对接
- UI 监听 `USkillTreeComponent::OnSkillPointChanged`；属性集需要广播变化值（事件已声明，注意绑定时机）。

7）复制与 UI 刷新顺序
- 带 `OnRep` 的数组按变更触发；UI 期望“先全量刷新，再增量更新”。绑定 UI 时记得调用 `RequestRefresh()`（`USkillTreeWidget` 已调用）。

8）冷却
- 冷却基于时间（`World->GetTimeSeconds()`）；务必在服务器端有有效的世界上下文与计时器。

## 验证（快速测试）

1）背包基础
- 用 Listen Server 启动 PIE。
- 通过调试或蓝图调用 `Inventory.AddItem(<ItemData>, X)`；观察 UI 是否变化。
- 在格位间移动物品；检查数量/堆叠是否正确。

2）冷却生命周期
- 创建一个带 `GrantedAbility` 且 `CooldownDuration>0` 的物品。
- 使用该物品 → 应触发 `OnInventoryItemCooldownStarted(ItemId, Duration, EndTime)`。
- 冷却结束 → 触发 `OnInventoryItemCooldownEnded(ItemId)`，UI 刷新。

3）装备属性加成
- 创建一件装备，设置 `TargetAttribute=MaxHealth`，`Magnitude=+50`。
- 通过 `UEquipmentComponent.EquipItemDirect` 或从背包装备。
- 验证 MaxHealth 提升；卸下后恢复。

4）掉落
- 给敌人添加 `ULootDropComponent`，设置一个必掉项（`DropChance=1`）的 `LootTable`。
- 将敌人生命值降到 0 → 生成拾取；玩家重叠拾取 → 背包获得物品；数量消耗完毕拾取销毁。

5）技能树解锁路径
- 创建简单树：Root（cost=1）→ Child（cost=1，前置=Root）。
- 给玩家 2 点技能点（通过属性或调试）。
- 点击 Root 解锁；再点击 Child 解锁；技能点为 0；UI 同步更新。

6）设置菜单快速检查
- 在 WBP_SettingsMenu 中调用：`SetMasterVolume(0.5)`、`SetFrameRateLimit(60)`、`SetDisplayGamma(2.2)`；确认效果生效。

## 回滚 / 恢复方案

- 已创建安全快照分支：`backup/copilot-20251101`。
- 想回到改动前：检出你之前的分支/提交；需要的文件可从备份分支按需挑回（cherry-pick/复制）。

## 建议的渐进式集成顺序
1）背包核心（物品 + UI）→ 构建 → 测试
2）装备（应用/移除加成）→ 构建 → 测试
3）掉落（生成 → 拾取）→ 构建 → 测试
4）技能树（数据 + 运行时 + UI）→ 构建 → 测试
5）设置 UI、TypeScript 行为树任务 → 可最后再接入
