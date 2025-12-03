import * as UE from "ue";
import { blueprint } from "puerts";

class BTT_GenericAttack extends UE.BTTask_TSBase {
    
    // 任务执行时
    ReceiveExecuteAI(OwnerController: UE.AIController, ControlledPawn: UE.Pawn): void {
        
        // 1. 获取技能类
        let abilityClass: UE.TSubclassOf<UE.GameplayAbility> = null;
        
        // 尝试从 TS_BaseEnemy (或兼容对象) 获取配置
        const enemy = ControlledPawn as any;
        
        // 优先使用 GetAttackAbilityClass 接口 (读取 C++ DefaultAbilities)
        if (typeof enemy.GetAttackAbilityClass === 'function') {
             abilityClass = enemy.GetAttackAbilityClass();
        }
        // 兼容旧的直接属性访问 (如果用户还没删)
        else if (enemy.AttackAbilityClass) {
            abilityClass = enemy.AttackAbilityClass;
        }

        if (!abilityClass) {
            console.warn(`[BTT_GenericAttack] Pawn ${ControlledPawn.GetName()} has no Attack Ability configured! Check DefaultAbilities.`);
            this.FinishExecute(false);
            return;
        }

        // 2. 准备目标数据 (关键步骤：解决 GA_TargetedAbility 依赖 TargetData 的问题)
        // 从 Blackboard 获取目标 Actor
        const bb = OwnerController.Blackboard;
        const targetActor = bb ? bb.GetValueAsObject("TargetActor") as UE.Actor : null;

        if (targetActor) {
            // 尝试调用 Character 的 SetAbilityTargetDataByActor
            // 这会将目标数据存入 StoredTargetData，供技能读取
            if (typeof enemy.SetAbilityTargetDataByActor === 'function') {
                enemy.SetAbilityTargetDataByActor(targetActor);
                // console.log(`[BTT_GenericAttack] Set TargetData to ${targetActor.GetName()}`);
            } else {
                console.warn(`[BTT_GenericAttack] Pawn ${ControlledPawn.GetName()} does not support SetAbilityTargetDataByActor!`);
            }
        } else {
            console.warn(`[BTT_GenericAttack] No TargetActor in Blackboard!`);
        }

        // 3. 通过 GAS 系统激活技能
        // 尝试获取 ASC
        let asc = enemy.AbilitySystemComponent as UE.AbilitySystemComponent;
        console.warn(`[BTT_GenericAttack] Retrieved ASC: ${asc}`);
        if (!asc) {
            asc = ControlledPawn.GetComponentByClass(UE.AbilitySystemComponent.StaticClass()) as UE.AbilitySystemComponent;
        }

        if (asc) {
            // 直接通过类激活技能
            // PuerTS 中 TSubclassOf 需要转换为 Class 类型传递
            const bActivated = asc.TryActivateAbilityByClass(abilityClass as unknown as UE.Class, true);
            
            if (bActivated) {
                // 成功激活
                this.FinishExecute(true);
            } else {
                // 激活失败（冷却中等），返回 true 以进入 Wait 状态
                console.log(`[BTT_GenericAttack] Ability not activated (Cooldown?), waiting...`);
                this.FinishExecute(true);
            }
        } else {
            console.warn("[BTT_GenericAttack] No ASC found on enemy!");
            this.FinishExecute(false);
        }
    }
}

// --- Mixin 补丁 ---
// 由于行为树编辑器无法识别直接继承自 TS 类的蓝图，
// 我们采用 "Mixin" 模式：
// 1. 让蓝图继承自 C++ 的 BTTask_TSBase (这样编辑器能看到它)
// 2. 在运行时把 TS 的逻辑注入到那个蓝图里
export function ApplyMixin() {
    // 请确保这个路径和你的蓝图路径一致
    // 假设蓝图名为 BP_BTT_GenericAttack，位于 Content/AI 目录下
    const bpPath = "/Game/AI/BTT_GenericAttack.BTT_GenericAttack_C";
    
    try {
        const ucls = UE.Class.Load(bpPath);
        if (ucls) {
            // 将蓝图类转换为 PuerTS 可识别的 JS 类
            const BP_Class = blueprint.tojs<typeof UE.BTTask_TSBase>(ucls);
            
            // 执行 Mixin：将 BTT_GenericAttack 的方法注入到 BP_Class 中
            blueprint.mixin(BP_Class, BTT_GenericAttack);
            
            console.log(`[BTT_GenericAttack] Mixin applied to ${bpPath}`);
        } else {
            console.warn(`[BTT_GenericAttack] Blueprint not found at ${bpPath}. Make sure the asset exists and is compiled.`);
        }
    } catch (e) {
        console.warn(`[BTT_GenericAttack] Mixin failed: ${e}`);
    }
}

export default BTT_GenericAttack;
