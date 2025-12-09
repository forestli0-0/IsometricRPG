import * as UE from "ue";
import { ApplyMixin as ApplyAttackMixin } from "./BTT_GenericAttack";

// 尝试应用 Mixin (确保 BTT 逻辑被注入到蓝图中)
ApplyAttackMixin();

// 黑板键名常量
const BBKEY_SELF_ACTOR = "SelfActor";
const BBKEY_TARGET_ACTOR = "TargetActor";
const BBKEY_LAST_KNOWN_LOCATION = "LastKnownLocation";
const BBKEY_IS_IN_ATTACK_RANGE = "bIsInAttackRange";

class TS_EnemyAIController extends UE.IsometricAIController {
    // --- 属性 ---
    LoseSightDistance: number = 2000;

    // @UE.uproperty.uproperty(UE.uproperty.VisibleAnywhere, UE.uproperty.BlueprintReadWrite, "Category=Combat")
    // AttackRange: number = 200;

    @UE.uproperty.uproperty(UE.uproperty.EditAnywhere, UE.uproperty.BlueprintReadWrite, "Category=Debug")
    EnableDebug: boolean = true;

    // Tick 节流
    UpdateInterval: number = 0.25;
    private _accum: number = 0;

    // --- 生命周期 ---

    ReceivePossess(InPawn: UE.Pawn): void {
        super.ReceivePossess(InPawn);
    }

    ReceiveBeginPlay(): void {
        // 1. 运行行为树
        if (this.BehaviorTreeAsset) {
            // TS 定义中 RunBehaviorTree 返回 bool
            const bSuccess = this.RunBehaviorTree(this.BehaviorTreeAsset);
            if(this.EnableDebug) console.log(`[AI] BT Start: ${bSuccess}`);
        }

        // 2. 设置 SelfActor
        const pawn = this.Pawn;
        const bb = this.Blackboard;
        
        if (pawn && bb) {
            bb.SetValueAsObject(BBKEY_SELF_ACTOR, pawn);
        }

        // 3. 绑定感知组件
        // 优先查找 Controller 上的感知组件
        let perceptionComp = this.GetComponentByClass(UE.AIPerceptionComponent.StaticClass()) as UE.AIPerceptionComponent;
        
        // 如果 Controller 上没有，尝试查找 Pawn 上的感知组件 (兼容 C++ Character 定义的情况)
        if (!perceptionComp && pawn) {
            perceptionComp = pawn.GetComponentByClass(UE.AIPerceptionComponent.StaticClass()) as UE.AIPerceptionComponent;
        }

        if (perceptionComp) {
            perceptionComp.OnTargetPerceptionUpdated.Add((Actor, Stimulus) => {
                this.HandlePerceptionUpdated(Actor, Stimulus);
            });
            if (this.EnableDebug) console.log(`[AI] Perception Component bound on ${this.GetName()} (Source: ${perceptionComp.GetOwner().GetName()})`);
        } else {
            console.warn(`[AI] Warning: No AIPerceptionComponent found on Controller or Pawn!`);
        }
    }

    ReceiveTick(DeltaSeconds: number): void {
        // 节流更新，避免每帧计算
        this._accum += DeltaSeconds;
        if (this._accum < this.UpdateInterval) return;
        this._accum = 0;

        const bb = this.Blackboard;
        const pawn = this.Pawn;
        if (!bb || !pawn) return;

        const targetActor = bb.GetValueAsObject(BBKEY_TARGET_ACTOR) as UE.Actor;
        if (targetActor) {
            // 1. 更新目标位置 (让 AI 追踪移动中的目标)
            const targetLoc = targetActor.K2_GetActorLocation();
            bb.SetValueAsVector(BBKEY_LAST_KNOWN_LOCATION, targetLoc);

            // 2. 检查攻击距离
            const dist = UE.Vector.Dist(pawn.K2_GetActorLocation(), targetLoc);
            
            // 获取实际攻击距离 (从 AttributeSet)
            let attackRange = 200; // 默认值
            if (typeof (pawn as any).GetAttackRange === 'function') {
                attackRange = (pawn as any).GetAttackRange();
            }

            // 考虑到胶囊体半径，实际判定距离应该稍微宽容一点
            // 简单的做法是：距离 <= 攻击距离 + 容差
            const bInRange = dist <= (attackRange + 50); // +50 容差

            bb.SetValueAsBool(BBKEY_IS_IN_ATTACK_RANGE, bInRange);

            // Debug
            // if (this.EnableDebug) console.log(`[AI] Tick: Dist=${Math.floor(dist)} Range=${attackRange} InRange=${bInRange}`);
        } else {
            bb.SetValueAsBool(BBKEY_IS_IN_ATTACK_RANGE, false);
        }
    }

    ReceiveEndPlay(EndPlayReason: UE.EEndPlayReason): void {
        // 当前不在控制器里绑定感知委托，无需清理
    }

    // --- 核心逻辑 ---

    /**
     * 处理感知更新 (替代原来的 OnPerceptionUpdated)
     * 利用 AIStimulus 判断是“看见”还是“丢失”
     */
    private HandlePerceptionUpdated(Actor: UE.Actor, Stimulus: UE.AIStimulus): void {
        const bb = this.Blackboard;
        const pawn = this.Pawn;
        if (!bb || !pawn) return;

        // bSuccessfullySensed: true=感知到，false=失去感知
        if (Stimulus.bSuccessfullySensed) {
            // 目标可见：写入目标与当前位置
            const targetLoc = Actor ? Actor.K2_GetActorLocation() : Stimulus.StimulusLocation;
            
            bb.SetValueAsObject(BBKEY_TARGET_ACTOR, Actor);
            bb.SetValueAsVector(BBKEY_LAST_KNOWN_LOCATION, targetLoc);
            
            // 距离检查移到 Tick 中统一处理，这里只负责“发现目标”
            if (this.EnableDebug) console.log(`[AI] Sensed: ${Actor.GetName()}`);
        } else {
            // 目标丢失：记录“最后一次感知位置”，并在仍追踪同一Actor时清空目标
            bb.SetValueAsVector(BBKEY_LAST_KNOWN_LOCATION, Stimulus.StimulusLocation);

            const currentTarget = bb.GetValueAsObject(BBKEY_TARGET_ACTOR) as UE.Actor;
            if (currentTarget === Actor) {
                bb.SetValueAsObject(BBKEY_TARGET_ACTOR, null);
                bb.SetValueAsBool(BBKEY_IS_IN_ATTACK_RANGE, false);
                if (this.EnableDebug) console.log(`[AI] Lost: ${Actor.GetName()}`);
            }
        }
    }

    /**
     * 供角色蓝图调用：把感知事件转发到控制器
     * BP 签名：OnPerceptionUpdated(Target, SensedActor)
     */
    OnPerceptionUpdated(Target: UE.Actor, SensedActor: UE.AIStimulus): void {
        this.HandlePerceptionUpdated(Target, SensedActor);
    }
}

export default TS_EnemyAIController;