import * as UE from "ue";


/**
 * 将本脚本挂在场景中任意位置，用于批量生成敌人进行性能对比。
 * - 可配置敌人类（必须是 Character 的蓝图或类）
 * - 可选覆盖 AIControllerClass（用于切换 TS/蓝图控制器）
 * - 默认使用事件驱动的 AutoPossess AI（PlacedInWorldOrSpawned）
 * - 生成完成后打印耗时，便于配合 `stat unit` 观察
 */
class TS_EnemyBatchSpawner extends UE.Actor {
  // 生成配置
// Category 用于在细节面板分组

    EnemyClass: UE.Class; 

    EnemyClassAssetPath: string; 


    AIControllerClassOverride: UE.Class; 


    AIControllerClassAssetPath: string;

    // --- 数值配置 ---


    NumToSpawn: number = 1000; 
    GridColumns: number = 50; 
    Spacing: number = 200; 
    RandomJitter: number = 0; 

    // --- 开关配置 ---

    bSpawnOnBeginPlay: boolean = true;
    bDestroyOnEndPlay: boolean = true; 
    bEnsureController: boolean = true; 
    bOverrideFromActorTags: boolean = true;

  private _spawned: UE.Actor[] = [];

  Constructor() {
    // 明确在构造函数中重复设定默认值，确保初始化执行
    this.NumToSpawn = 100;
    this.GridColumns = 50;
    this.Spacing = 200;
    this.RandomJitter = 0;
    this.bSpawnOnBeginPlay = true;
    this.bDestroyOnEndPlay = true;
    this.bEnsureController = true;
    this.bOverrideFromActorTags = true;
    // 确保数组初始化（有时属性初始化可能在热重载后丢失）
    this._spawned = [];
  }

  ReceiveBeginPlay(): void {
    console.log("[Spawner] BeginPlay triggered.");
    this.debugDump("InitialBeforeTags");

    // 可选：从标签覆盖配置
    if (this.bOverrideFromActorTags) {
      this.applyOverridesFromTags();
      this.debugDump("AfterTags");
    }

    if (!this.bSpawnOnBeginPlay) {
      console.log("[Spawner] bSpawnOnBeginPlay is false, skipping spawn.");
      return;
    }

    if (!this.EnemyClass && !this.EnemyClassAssetPath) {
      console.warn("[Spawner] EnemyClass/EnemyClassAssetPath 均未设置，终止生成。");
      return;
    }

    const startMs = Date.now();
    const spawned = this.spawnBatch();
    const cost = Date.now() - startMs;

    console.log(`[Spawner] Spawned ${spawned} actors in ${cost} ms.`);
  }

  ReceiveEndPlay(Reason: UE.EEndPlayReason): void {
    if (!this.bDestroyOnEndPlay) return;
    for (const a of this._spawned) {
      if (a) a.K2_DestroyActor();
    }
    this._spawned.length = 0;
  }

  private spawnBatch(): number {
    const world = this.GetWorld();
    if (!world) return 0;

    const origin = this.K2_GetActorLocation();
    const cols = Math.max(1, Math.floor(this.GridColumns));
    const count = Math.max(0, Math.floor(this.NumToSpawn));
    const spacing = Math.max(1, this.Spacing);
    const jitter = Math.max(0, this.RandomJitter);

    let spawned = 0;
    // 防御：若热重载导致 _spawned 丢失，重新初始化
    if (!this._spawned) this._spawned = [];
    for (let i = 0; i < count; i++) {
      const r = Math.floor(i / cols);
      const c = i % cols;

      const baseX = origin.X + r * spacing;
      const baseY = origin.Y + c * spacing;
      const baseZ = origin.Z;

      const x = baseX + (jitter > 0 ? (Math.random() * 2 - 1) * jitter : 0);
      const y = baseY + (jitter > 0 ? (Math.random() * 2 - 1) * jitter : 0);
      const z = baseZ;

      const loc = new UE.Vector(x, y, z);
      const rot = new UE.Rotator(0, 0, 0);
      const scl = new UE.Vector(1, 1, 1);
      const tf = new UE.Transform(rot, loc, scl);

      const actor = this.spawnOne(tf);
      if (actor) {
        // 再次防御，保证 push 安全
        if (!this._spawned) this._spawned = [];
        this._spawned.push(actor);
        spawned++;
      }
    }
    return spawned;
  }

  private spawnOne(tf: UE.Transform): UE.Actor | null {
    // 使用延迟生成以便在 FinishSpawning 之前设置属性
    const enemyClass = this.resolveEnemyClass();
    if (!enemyClass) {
      console.warn("[Spawner] 无法解析 EnemyClass（请设置 EnemyClass 或 EnemyClassAssetPath）。");
      return null;
    }

    const deferred = UE.GameplayStatics.BeginDeferredActorSpawnFromClass(
      this,
      enemyClass,
      tf,
      UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn,
      null,
      null
    );

    if (!deferred) return null;

    // 期望是 Character（运行时再做一次保护性校验）
    if (!this.isChildOf(enemyClass, UE.Character.StaticClass())) {
      console.warn(`[Spawner] EnemyClass 非 Character 子类：${this.EnemyClassAssetPath || enemyClass.GetName()}`);
      return null;
    }
    const ch = deferred as UE.Character;

    // 覆盖 ControllerClass（可选）
    const overrideCtrlClass = this.resolveAIControllerClass();
    if (overrideCtrlClass) {
      (ch as any).AIControllerClass = overrideCtrlClass;
    }

    // 确保 AutoPossess AI 为“放置或生成时”
    (ch as any).AutoPossessAI = UE.EAutoPossessAI.PlacedInWorldOrSpawned;

    // 完成生成
    UE.GameplayStatics.FinishSpawningActor(ch, tf);

    // 若未被控制器接管，则强制生成默认控制器
    if (this.bEnsureController && !(ch.Controller)) {
      // 仅在 Pawn/Character 上调用
      const spawnFunc = (ch as any).SpawnDefaultController;
      if (typeof spawnFunc === 'function') {
        spawnFunc.call(ch);
      } else {
        console.warn('[Spawner] SpawnDefaultController 不可用，确保 EnemyClass 派生自 Pawn/Character。');
      }
    }

    return ch;
  }

  private resolveEnemyClass(): UE.Class | null {
    // 优先使用蓝图类路径
    if (this.EnemyClassAssetPath && this.EnemyClassAssetPath.length > 0) {
      try {
        const cls = (UE.Class as any).Load(this.EnemyClassAssetPath);
        if (cls) {
          // 验证继承关系
          if (!this.isChildOf(cls as UE.Class, UE.Character.StaticClass())) {
            console.warn(`[Spawner] 路径指向的类并非 Character 子类：${this.EnemyClassAssetPath}`);
            return null;
          }
          return cls as UE.Class;
        }
      } catch (e) {
        console.warn(`[Spawner] 加载 EnemyClass 失败: ${this.EnemyClassAssetPath}`, e);
      }
    }
    // 回退到原生类
    if (this.EnemyClass) {
      if (!this.isChildOf(this.EnemyClass, UE.Character.StaticClass())) {
        console.warn('[Spawner] EnemyClass 非 Character 子类。');
        return null;
      }
      return this.EnemyClass;
    }
    return null;
  }

  private resolveAIControllerClass(): UE.Class | null {
    if (this.AIControllerClassAssetPath && this.AIControllerClassAssetPath.length > 0) {
      try {
        const cls = (UE.Class as any).Load(this.AIControllerClassAssetPath);
        if (cls) {
          if (!this.isChildOf(cls as UE.Class, UE.AIController.StaticClass())) {
            console.warn(`[Spawner] 路径指向的类并非 AIController 子类：${this.AIControllerClassAssetPath}`);
            return null;
          }
          return cls as UE.Class;
        }
      } catch (e) {
        console.warn(`[Spawner] 加载 AIControllerClass 失败: ${this.AIControllerClassAssetPath}`, e);
      }
    }
    if (this.AIControllerClassOverride) {
      if (!this.isChildOf(this.AIControllerClassOverride, UE.AIController.StaticClass())) {
        console.warn('[Spawner] AIControllerClassOverride 非 AIController 子类。');
        return null;
      }
      return this.AIControllerClassOverride;
    }
    return null;
  }

  private isChildOf(cls: UE.Class | null, parent: UE.Class): boolean {
    try {
      return !!cls && typeof (cls as any).IsChildOf === 'function' && (cls as any).IsChildOf(parent);
    } catch {
      return false;
    }
  }

  // --- 配置覆盖：从 Actor Tags 读取 ---
  // 支持的键（大小写不敏感）：
  //  Enemy=/Game/Path/BP_Enemy.BP_Enemy_C
  //  AIC=/Game/Path/BP_Controller.BP_Controller_C
  //  Num=1000  Cols=50  Spacing=200  Jitter=0
  //  Ensure=1  Begin=1
  private applyOverridesFromTags(): void {
    const tags: any = (this as any).Tags;
    if (!tags || typeof tags.Num !== 'function') return;

    const n = tags.Num();
    for (let i = 0; i < n; i++) {
      const raw = tags.Get(i);
      const t = (raw && (raw.ToString ? raw.ToString() : raw.toString ? raw.toString() : String(raw))) as string;
      if (!t || t.indexOf('=') === -1) continue;

      const idx = t.indexOf('=');
      const k = t.substring(0, idx).trim().toLowerCase();
      const v = t.substring(idx + 1).trim();

      switch (k) {
        case 'enemy':
        case 'enemyclass':
        case 'enemyclassassetpath':
          this.EnemyClassAssetPath = v;
          break;
        case 'aic':
        case 'aiclass':
        case 'aicontrollerclassassetpath':
          this.AIControllerClassAssetPath = v;
          break;
        case 'num':
          this.NumToSpawn = this.toInt(v, this.NumToSpawn);
          break;
        case 'cols':
        case 'columns':
          this.GridColumns = this.toInt(v, this.GridColumns);
          break;
        case 'spacing':
          this.Spacing = this.toFloat(v, this.Spacing);
          break;
        case 'jitter':
          this.RandomJitter = this.toFloat(v, this.RandomJitter);
          break;
        case 'ensure':
          this.bEnsureController = this.toBool(v, this.bEnsureController);
          break;
        case 'begin':
          this.bSpawnOnBeginPlay = this.toBool(v, this.bSpawnOnBeginPlay);
          break;
      }
    }
  }

  private toInt(v: string, def: number): number {
    const n = parseInt(v, 10);
    return Number.isFinite(n) ? n : def;
  }
  private toFloat(v: string, def: number): number {
    const n = parseFloat(v);
    return Number.isFinite(n) ? n : def;
  }
  private toBool(v: string, def: boolean): boolean {
    const s = v.trim().toLowerCase();
    if (s === '1' || s === 'true' || s === 'yes' || s === 'on') return true;
    if (s === '0' || s === 'false' || s === 'no' || s === 'off') return false;
    return def;
  }

  private debugDump(tag: string): void {
    console.log(`[SpawnerDebug:${tag}] SpawnOnBeginPlay=${this.bSpawnOnBeginPlay} OverrideTags=${this.bOverrideFromActorTags} Num=${this.NumToSpawn} Cols=${this.GridColumns} Spacing=${this.Spacing} Jitter=${this.RandomJitter} EnemyClassAssetPath=${this.EnemyClassAssetPath} AIControllerClassAssetPath=${this.AIControllerClassAssetPath}`);
  }
}

export default TS_EnemyBatchSpawner;
