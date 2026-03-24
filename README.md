# IsometricRPG

> 这是一个 UE5 等角视角 ARPG 战斗原型项目。  

## 项目定位

- 项目类型：UE5 等角视角 ARPG gameplay 原型
- 当前重心：战斗交互主链路、能力系统抽象、固定等角相机、HUD 联动、基础同步与可扩展性

## 我主要做了什么

### 1. 输入与战斗交互主链路

- 用 `Enhanced Input` 统一接入左键选中、右键交互、`Q/W/E/R/D/F` 技能输入。
- 在 `PlayerController` 中做鼠标射线检测，把屏幕输入转换为世界空间 `HitResult`。
- 右键不是固定动作，而是统一意图入口：
  - 点地面时触发移动
  - 点敌人时分流到普通攻击
  - 技能按键会结合当前鼠标命中结果生成目标数据
- 对“按住右键”的输入做了节流处理，避免每帧都直接发命令。

### 2. 点击移动与路径跟随

- 没有直接把点击移动完全交给引擎黑盒，而是自己实现了 `UIsometricPathFollowingComponent`。
- 这层负责点击目标、同步寻路、路径点推进和重寻路决策。
- 真正的位移仍然交给 `CharacterMovementComponent`，保留 UE 自带的移动、碰撞和网络同步链路。
- 玩家角色的路径执行放在 owning client，本地响应更直接，服务端保留权威状态处理。

### 3. 基于 GAS 的技能系统框架

- 采用 `PlayerState + AbilitySystemComponent` 来承载角色属性和技能授予。
- 抽了统一的英雄技能基类，收敛公共逻辑：
  - 冷却
  - 消耗
  - 蒙太奇播放
  - 目标数据读取
  - 提交与取消
- 已经抽象出多种施法类型：
  - `Targeted`
  - `SkillShot`
  - `Projectile`
  - `SelfCast`
  - `AreaEffect`
  - `Melee`
  - `Passive`
- 技能输入和目标数据拆开处理：输入决定激活哪个能力，目标信息通过 `GameplayAbilityTargetData` 传给能力执行层。

### 4. 属性、UI 与反馈联动

- HUD 侧拆了 `ActionBar`、`StatusPanel`、`ResourcePanel` 等模块。
- 技能栏支持槽位展示、图标刷新、冷却反馈、资源消耗信息展示。
- 属性变化、经验变化、等级变化、状态 Tag 变化都会回推到 HUD。
- 用 `PlayerState` 统一管理技能槽位、能力授予结果和 UI 刷新时机，减少角色本体和 UI 的直接耦合。

### 5. AI 与脚本扩展

- 项目内有基础敌人 AI 控制器、行为树任务和感知组件接入。
- 接了 `PuerTS`，用 TypeScript 写了一部分 AI 与批量生成逻辑，方便快速迭代原型。
- 当前仓库里可以看到的 TS 脚本主要集中在：
  - 敌人 AI Controller
  - 通用攻击行为树逻辑
  - 敌人批量生成

## 当前已经落地的内容

- 左键选中目标、右键移动/攻击分流
- 固定等角相机与角色控制
- 等角视角角色控制与点击移动
- 普攻技能和多类主动技能框架
- 基于 GAS 的冷却、消耗、目标数据传递
- 技能栏、状态面板、资源面板、经验反馈
- 经验球与基础等级成长反馈
- 基础 AI 感知、行为树任务与敌人战斗逻辑
- 基础 RPC、属性复制、`OnRep` 和目标数据同步

## 技术栈

- `Unreal Engine 5.5`
- `C++`
- `Gameplay Ability System`
- `Enhanced Input`
- `UMG`
- `AI Module / Behavior Tree / AI Perception`
- `Niagara`
- `Blueprint`
- `PuerTS / TypeScript`

## 仓库结构

```text
Source/       核心 C++ 代码，包含角色、输入、技能、UI、AI、升级系统等模块
Content/      地图、蓝图、UI、FX、输入资源和游戏内容
Config/       项目配置、输入配置、GameplayTag、Puerts 配置
TypeScript/   PuerTS 脚本，主要用于 AI 与原型辅助逻辑
Plugins/      项目使用到的插件（含 Puerts / UnLua）
docs/         设计记录、系统拆解和项目复盘文档
```

## 如何运行

### 环境

- `Unreal Engine 5.5`
- `Visual Studio 2022` 或可用的 UE C++ 编译环境
- Windows

### 启动方式

1. 用 Unreal Engine 5.5 打开 [IsometricRPG.uproject](./IsometricRPG.uproject)
2. 首次打开时按提示编译 C++ 模块
3. 进入项目后打开 `Content/Maps/DevelopmentMap`
4. 运行 PIE 进行功能验证

### 如果需要改 TypeScript 脚本

```bash
npm install
npm run build
```

## 当前阶段与边界

- 这是以 gameplay 和系统设计为主的个人原型项目，不是完整商业化内容项目。
- 重点已经放在战斗交互闭环和能力系统抽象上，地图内容、美术资源和数值打磨不是当前主线。
- 联机部分目前是“基础同步可跑通”的状态，重心在 RPC、权威执行和状态同步，还没有往完整预测/回滚架构继续深入。
- 仓库里同时保留了一些实验性插件和脚本扩展，用来验证不同的原型迭代方式。

## 额外说明

- 这个项目是我用来系统梳理 UE Gameplay 开发思路的一套练手工程，所以仓库里会保留不少拆分模块、实验实现和复盘文档。
